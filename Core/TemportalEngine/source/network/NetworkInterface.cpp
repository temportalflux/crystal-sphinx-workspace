#include "network/NetworkInterface.hpp"

#include "game/GameInstance.hpp"
#include "network/NetworkCore.hpp"
#include "network/NetworkPacket.hpp"
#include "utility/Casting.hpp"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

using namespace network;

SteamNetworkingConfigValue_t makeConfigCallback(ESteamNetworkingConfigValue key, network::Interface *interface, void (network::Interface::*f)(void*))
{
	std::function<void(void*)> callback = std::bind(f, interface, std::placeholders::_1);
	SteamNetworkingConfigValue_t option = {};
	option.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, callback.target<void(void*)>());
	return option;
}

Interface::Interface() : mpInternal(nullptr), mType(EType::eInvalid), mConnection(0)
{
}

Interface& Interface::setType(EType type)
{
	this->mType = type;
	return *this;
}

Interface& Interface::setAddress(Address const& address)
{
	this->mAddress = address;
	return *this;
}

void connectionCallback(SteamNetConnectionStatusChangedCallback_t *pInfo)
{
	auto& netInterface = game::Game::Get()->networkInterface();
	switch (netInterface.type())
	{
	case network::EType::eServer:
		netInterface.onServerConnectionStatusChanged((void*)pInfo);
		break;
	case network::EType::eClient:
		netInterface.onClientConnectionStatusChanged((void*)pInfo);
		break;
	default: break;
	}
}

void Interface::start()
{
	if (this->mType == EType::eInvalid) return;

	this->mpInternal = SteamNetworkingSockets();
	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);

	auto address = *as<SteamNetworkingIPAddr>(this->mAddress.get());
	auto options = std::vector<SteamNetworkingConfigValue_t>();

	SteamNetworkingConfigValue_t option = {};
	option.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, &connectionCallback);
	options.push_back(option);

	if (this->mType == EType::eServer)
	{
		network::logger().log(LOG_INFO, "Listening at IP %s", this->mAddress.toString(true).c_str());
		this->mConnection = pInterface->CreateListenSocketIP(address, (ui32)options.size(), options.data());
		if (this->mConnection == k_HSteamListenSocket_Invalid)
		{
			network::logger().log(LOG_ERR, "Failed to create listen socket on port %d", this->mAddress.port());
			return;
		}
		
		this->mServerPollGroup = pInterface->CreatePollGroup();
		if (this->mServerPollGroup == k_HSteamNetPollGroup_Invalid)
		{
			network::logger().log(LOG_ERR, "Failed to create listen socket on port %d", this->mAddress.port());
			return;
		}
	}
	else if (this->mType == EType::eClient)
	{
		network::logger().log(LOG_INFO, "Connecting to server at %s", this->mAddress.toString(true).c_str());
		this->mConnection = pInterface->ConnectByIPAddress(address, (ui32)options.size(), options.data());
		if (this->mConnection == k_HSteamNetConnection_Invalid)
		{
			network::logger().log(LOG_ERR, "Failed to create connection to %s", this->mAddress.toString(true).c_str());
			return;
		}
	}

}

void Interface::stop()
{
	if (this->mpInternal == nullptr) return;

	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	if (this->mType == EType::eServer)
	{
		network::logger().log(LOG_INFO, "Server is shutting down.");

		for (auto const& [clientId,identity] : this->mClients)
		{
			pInterface->CloseConnection(clientId, 0, "Server Shutdown", true);
		}
		this->mClients.clear();

		if (this->mConnection != k_HSteamListenSocket_Invalid)
		{
			pInterface->CloseListenSocket(this->mConnection);
			this->mConnection = k_HSteamListenSocket_Invalid;
		}

		if (this->mServerPollGroup != k_HSteamNetPollGroup_Invalid)
		{
			pInterface->DestroyPollGroup(this->mServerPollGroup);
			this->mServerPollGroup = k_HSteamNetPollGroup_Invalid;
		}
	}
	else if (this->mType == EType::eClient)
	{
		if (this->mConnection != k_HSteamNetConnection_Invalid)
		{
			pInterface->CloseConnection(this->mConnection, 0, nullptr, false);
			this->mConnection = k_HSteamNetConnection_Invalid;
		}
	}

	this->mpInternal = nullptr;
}

bool Interface::hasConnection() const
{
	switch (this->mType)
	{
	case EType::eClient: return this->mConnection != k_HSteamNetConnection_Invalid;
	case EType::eServer: return this->mConnection != k_HSteamListenSocket_Invalid;
	default: return false;
	}
}

void Interface::update(f32 deltaTime)
{
	if (this->mpInternal == nullptr) return;
	if (!this->hasConnection()) return;
	
	this->pollIncomingMessages();
	
	while (this->mReceivedPackets.size() > 0)
	{
		auto iter = this->mReceivedPackets.begin();
		(*iter)->process(this);
		this->mReceivedPackets.erase(iter);
	}
	
	as<ISteamNetworkingSockets>(this->mpInternal)->RunCallbacks();
}

void Interface::pollIncomingMessages()
{
	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	ISteamNetworkingMessage *pIncomingMsg = nullptr;
	i32 numMsgs = -1;

	if (this->mType == EType::eServer)
	{
		numMsgs = pInterface->ReceiveMessagesOnPollGroup(this->mServerPollGroup, &pIncomingMsg, 1);
	}
	else if (this->mType == EType::eClient)
	{
		numMsgs = pInterface->ReceiveMessagesOnConnection(this->mConnection, &pIncomingMsg, 1);
	}
	else
	{
		assert(false);
	}

	if (numMsgs == 0) return;
	else if (numMsgs < 0)
	{
		network::logger().log(LOG_ERR, "Failed to poll for messages.");
		return;
	}
	assert(numMsgs == 1 && pIncomingMsg);
	
	auto data = reinterpret_cast<Packet::Data*>(pIncomingMsg->m_pData);
	auto pPacket = this->packetTypes().create(data->packetTypeId);
	assert(pIncomingMsg->m_cbSize == pPacket->dataSize());
	pPacket->setSourceConnection(pIncomingMsg->m_conn);
	memcpy_s(pPacket->data(), pPacket->dataSize(), pIncomingMsg->m_pData, pPacket->dataSize());
	pIncomingMsg->Release();
	this->mReceivedPackets.push_back(pPacket);
}

void Interface::onServerConnectionStatusChanged(void* pInfo)
{
	assert(this->mType == EType::eServer);
	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	auto data = (SteamNetConnectionStatusChangedCallback_t*)pInfo;
	switch (data->m_info.m_eState)
	{
	// Happens when a server destroys a connection with a client
	case k_ESteamNetworkingConnectionState_None: break;

	case k_ESteamNetworkingConnectionState_Connecting:
	{
		if (pInterface->AcceptConnection(data->m_hConn) != k_EResultOK)
		{
			// Ensure the client is cleaned up if the connection accept failed
			pInterface->CloseConnection(data->m_hConn, 0, nullptr, false);
			network::logger().log(LOG_INFO, "Failed to accept connection, it may be already closed.");
			return;
		}

		if (!pInterface->SetConnectionPollGroup(data->m_hConn, this->mServerPollGroup))
		{
			pInterface->CloseConnection(data->m_hConn, 0, nullptr, false);
			network::logger().log(LOG_ERR, "Failed to assign a poll group.");
			return;
		}

		this->mClients.insert(std::make_pair(data->m_hConn, game::UserIdentity()));

		network::logger().log(
			LOG_INFO, "Accepted connection request from %s",
			data->m_info.m_szConnectionDescription
		);

		this->onConnectionEstablished.execute(this, data->m_hConn);
		break;
	}

	// NO-OP: happens immediately after we, the server, accept the connection
	case k_ESteamNetworkingConnectionState_Connected: break;

	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		// If the connection was not previously connected, then the user dropped before it could be processed
		if (data->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
		{
			const char *disconnectionCause;
			if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
			{
				disconnectionCause = "problem detected locally";
			}
			else
			{
				disconnectionCause = "closed by peer";
			}

			network::logger().log(
				LOG_INFO, "Connection %s %s, reason %d: %s",
				data->m_info.m_szConnectionDescription, disconnectionCause,
				data->m_info.m_eEndReason, data->m_info.m_szEndDebug
			);

			this->mClients.erase(this->mClients.find(data->m_hConn));
		}
		// Actually destroy the connection data in the API
		pInterface->CloseConnection(data->m_hConn, 0, nullptr, false);
		break;
	}

	default: break;
	}
}

void Interface::onClientConnectionStatusChanged(void* pInfo)
{
	assert(this->mType == EType::eClient);
	auto* pInterface = as<ISteamNetworkingSockets>(this->mpInternal);
	auto data = (SteamNetConnectionStatusChangedCallback_t*)pInfo;
	assert(data->m_hConn == this->mConnection || this->mConnection == k_HSteamNetConnection_Invalid);
	switch (data->m_info.m_eState)
	{
	// NO-OP: the client destroyed its connection
	case k_ESteamNetworkingConnectionState_None: break;

	// NO-OP: the client has started connecting
	case k_ESteamNetworkingConnectionState_Connecting: break;

	case k_ESteamNetworkingConnectionState_Connected:
	{
		network::logger().log(LOG_INFO, "Successfully connected to server.");
		this->onConnectionEstablished.execute(this, data->m_hConn);
		break;
	}

	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		if (data->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
		{
			// soft-todo: distinguish between a timeout, a rejected connection, or some other transport problem.
			network::logger().log(LOG_INFO, "Disconnected from server: %s", data->m_info.m_szEndDebug);
		}
		else if (data->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
		{
			network::logger().log(LOG_INFO, "Lost server connection: %s", data->m_info.m_szEndDebug);
		}
		else
		{
			// soft-todo: check the reason code for a normal disconnection
			network::logger().log(LOG_INFO, "Disconnected from server: %s", data->m_info.m_szEndDebug);
		}

		pInterface->CloseConnection(data->m_hConn, 0, nullptr, false);
		this->mConnection = k_HSteamNetConnection_Invalid;
		break;
	}

	default: break;
	}
}

void Interface::sendPackets(ui32 connection, std::vector<std::shared_ptr<Packet>> const& packets)
{
	OPTICK_EVENT();
	assert(this->mpInternal != nullptr);

	auto utils = SteamNetworkingUtils();
	auto messages = std::vector<SteamNetworkingMessage_t*>();
	for (auto pPacket : packets)
	{
		auto message = utils->AllocateMessage(pPacket->dataSize());
		message->m_conn = connection;
		message->m_nFlags = ui32(pPacket->flags().data());
		memcpy_s(message->m_pData, pPacket->dataSize(), pPacket->data(), pPacket->dataSize());
		messages.push_back(message);
	}
	
	// All messages ptrs are owned by the interface after this call (no need to free them).
	// network::Packet objects are either discarded at then end of this stack,
	// or soon after because of `shared_ptr`.
	as<ISteamNetworkingSockets>(this->mpInternal)->SendMessages((ui32)messages.size(), messages.data(), nullptr);
}

void Interface::broadcastPackets(std::vector<std::shared_ptr<Packet>> const& packets)
{
	OPTICK_EVENT();
	assert(this->mpInternal != nullptr);
	assert(this->type() == EType::eServer);
	for (auto const& [clientId, identity] : this->mClients)
	{
		this->sendPackets(clientId, packets);
	}
}

game::UserIdentity& Interface::findIdentity(ui32 connection)
{
	auto iter = this->mClients.find(connection);
	assert(iter != this->mClients.end());
	return iter->second;
}
