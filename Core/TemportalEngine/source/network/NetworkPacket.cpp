#include "network/NetworkPacket.hpp"

#include "game/GameInstance.hpp"
#include "network/NetworkInterface.hpp"

using namespace network;

std::vector<EPacketFlags> utility::EnumWrapper<EPacketFlags>::ALL = {
	EPacketFlags::eUnreliable,
	EPacketFlags::eNoNagle,
	EPacketFlags::eNoDelay,
	EPacketFlags::eReliable,
	EPacketFlags::eUseCurrentThread,
};

std::string utility::EnumWrapper<EPacketFlags>::to_string() const
{
	switch (value())
	{
		case EPacketFlags::eUnreliable: return "Unreliable";
		case EPacketFlags::eReliable: return "Reliable";
		case EPacketFlags::eNoNagle: return "NoNagle";
		case EPacketFlags::eNoDelay: return "NoDelay";
		case EPacketFlags::eUseCurrentThread: return "UseCurrentThread";
		default: return "invalid";
	}
}
std::string utility::EnumWrapper<EPacketFlags>::to_display_string() const { return to_string(); }

Packet::Packet(utility::Flags<EPacketFlags> flags)
	: mFlags(flags)
{
}

utility::Flags<EPacketFlags> const& Packet::flags() const
{
	return this->mFlags;
}

std::shared_ptr<Packet> Packet::finalize()
{
	return this->shared_from_this();
}

void Packet::sendToServer()
{
	auto network = game::Game::Get()->networkInterface();
	assert(network.type() == EType::eClient);
	network.sendPackets(network.connection(), { this->finalize() });
}

void Packet::broadcast()
{
	game::Game::Get()->networkInterface().broadcastPackets({ this->finalize() });
}