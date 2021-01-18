#include "ecs/view/ECViewManager.hpp"

#include "ecs/Core.hpp"
#include "network/packet/NetworkPacketECSReplicate.hpp"

using namespace ecs;
using namespace ecs::view;

bool Manager::ViewRecord::operator<(Manager::ViewRecord const& other) const
{
	if (typeId < other.typeId) return true;
	if (typeId == other.typeId) return objectId < other.objectId;
	return false;
}

bool Manager::ViewRecord::operator>(Manager::ViewRecord const& other) const
{
	if (typeId > other.typeId) return true;
	if (typeId == other.typeId) return objectId > other.objectId;
	return false;
}

Manager::Manager()
	: mPool(sizeof(View), ECS_MAX_VIEW_COUNT)
{
	// pools automatically free dynamic memory when they go out of scope
	this->mPool.allocateMemory();
}

std::string Manager::typeName(TypeId const& typeId) const
{
	return this->mRegisteredTypes[typeId].name;
}

IEVCSObject* Manager::createObject(TypeId const& typeId, Identifier const& netId)
{
	auto ptr = this->create(typeId);
	ptr->setNetId(netId);
	this->assignNetworkId(netId, ptr->id());
	return ptr;
}

IEVCSObject* Manager::getObject(TypeId const& typeId, Identifier const& netId)
{
	return this->get(this->getNetworkedObjectId(netId));
}

void Manager::destroyObject(TypeId const& typeId, Identifier const& netId)
{
	auto objectId = this->getNetworkedObjectId(netId);
	this->removeNetworkId(netId);
	this->destroy(typeId, objectId);
	ecs::Core::logger().log(
		LOG_VERBOSE, "Destroyed %s view %u net-id(%u)",
		this->typeName(typeId).c_str(), objectId, netId
	);
}

View* Manager::create(ViewTypeId const& typeId)
{
	this->mMutex.lock();

	auto& typeMeta = this->getTypeMetadata(typeId);
	
	uIndex objectId;
	auto ptr = reinterpret_cast<View*>(this->mPool.create(objectId));
	typeMeta.construct(ptr);
	ptr->setId(objectId);

	uIndex idxRecord = this->mAllocatedObjects.insert(ViewRecord { typeId, objectId, ptr });
	this->mObjectsById[objectId] = ptr;

	if (typeMeta.mCount == 0) typeMeta.mFirstAllocatedIdx = idxRecord;
	typeMeta.mCount++;

	for (uIndex nextTypeId = typeId + 1; nextTypeId < ECS_MAX_VIEW_TYPE_COUNT; ++nextTypeId)
	{
		this->mRegisteredTypes[nextTypeId].mFirstAllocatedIdx++;
	}

	if (ecs::Core::Get()->shouldReplicate())
	{
		ptr->setNetId(this->nextNetworkId());
		this->assignNetworkId(ptr->netId(), ptr->id());
		ecs::Core::Get()->replicateCreate()
			->setObjectEcsType(ecs::EType::eView)
			.setObjectTypeId(typeId)
			.setObjectNetId(ptr->netId())
			;
	}

	this->mMutex.unlock();
	return ptr;
}

View* Manager::get(Identifier const& id)
{
	return this->mObjectsById[id];
}

void Manager::destroy(ViewTypeId const& typeId, Identifier const& id)
{
	this->mMutex.lock();

	auto idxRecord = this->mAllocatedObjects.search([typeId, id](ViewRecord const& record) -> i8
	{
		// typeId <=> record.typeId
		auto typeComp = typeId < record.typeId ? -1 : (typeId > record.typeId ? 1 : 0);
		if (typeComp != 0) return typeComp;
		// pCreated->mId <=> record.objectId
		auto objComp = id < record.objectId ? -1 : (id > record.objectId ? 1 : 0);
		return objComp;
	});
	assert(idxRecord);
	this->mAllocatedObjects.remove(*idxRecord);
	
	auto* pObject = this->mObjectsById[id];
	bool bWasReplicated = pObject->isReplicated();
	auto netId = pObject->netId();
	this->mObjectsById[id] = nullptr;

	auto& typeMeta = this->getTypeMetadata(typeId);
	typeMeta.mCount--;
	if (typeMeta.mFirstAllocatedIdx == *idxRecord)
	{
		typeMeta.mFirstAllocatedIdx = typeMeta.mCount > 0 ? *idxRecord + 1 : 0;
	}

	for (uIndex nextTypeId = typeId + 1; nextTypeId < ECS_MAX_VIEW_TYPE_COUNT; ++nextTypeId)
	{
		this->mRegisteredTypes[nextTypeId].mFirstAllocatedIdx--;
	}

	this->mPool.destroy<View>(id);

	if (bWasReplicated)
	{
		this->removeNetworkId(netId);
		if (ecs::Core::Get()->shouldReplicate())
		{
			ecs::Core::Get()->replicateDestroy()
				->setObjectEcsType(ecs::EType::eView)
				.setObjectTypeId(typeId)
				.setObjectNetId(netId)
				;
		}
	}

	ecs::Core::logger().log(
		LOG_VERBOSE, "Destroyed %s view id(%u). WasReplicated:%s net(%u)",
		this->typeName(typeId).c_str(), id,
		bWasReplicated ? "true" : "false", netId
	);

	this->mMutex.unlock();
}

Manager::ViewIterable Manager::getAllOfType(ViewTypeId const &typeId)
{
	auto& typeMeta = this->getTypeMetadata(typeId);
	return Manager::ViewIterable(this, typeMeta.mFirstAllocatedIdx, typeMeta.mCount);
}

Manager::ViewRecord& Manager::getRecord(uIndex const& idxRecord)
{
	return this->mAllocatedObjects[idxRecord];
}
