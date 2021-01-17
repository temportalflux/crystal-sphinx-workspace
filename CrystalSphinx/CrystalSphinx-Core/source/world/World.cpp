#include "world/World.hpp"

#include "Engine.hpp"
#include "asset/BlockType.hpp"
#include "ecs/system/SystemPhysicsIntegration.hpp"
#include "physics/ChunkCollisionManager.hpp"
#include "physics/PhysicsMaterial.hpp"
#include "physics/PhysicsRigidBody.hpp"
#include "physics/PhysicsScene.hpp"
#include "physics/PhysicsShape.hpp"
#include "physics/PhysicsSystem.hpp"
#include "registry/VoxelType.hpp"
#include "saveData/SaveDataRegistry.hpp"
#include "world/WorldTerrain.hpp"

using namespace game;

World::World() : mpSaveInstance(nullptr)
{
}

void World::init(saveData::Instance *saveInstance)
{
	this->mpSaveInstance = saveInstance;
	this->mSaveData = world::SaveData(this->mpSaveInstance->worldSave());
	this->mSaveData.readFromDisk();

	//this->createVoxelTypeRegistry();

	this->mpPhysics = std::make_shared<physics::System>();
	this->mpPhysics->init(true);

	this->mpSystemPhysicsIntegration = std::make_shared<ecs::system::PhysicsIntegration>();
	engine::Engine::Get()->addTicker(this->mpSystemPhysicsIntegration);

	this->createWorld();
}

void World::uninit()
{
	this->destroyWorld();
	this->mpSystemPhysicsIntegration.reset();
	this->mpPhysics.reset();
	this->mpVoxelTypeRegistry.reset();
}

world::SaveData& World::saveData() { return this->mSaveData; }

std::shared_ptr<game::VoxelTypeRegistry> World::voxelTypeRegistry() { return this->mpVoxelTypeRegistry; }

void World::createVoxelTypeRegistry()
{
	static auto Log = DeclareLog("VoxelTypeRegistry", LOG_INFO);

	this->mpVoxelTypeRegistry = std::make_shared<game::VoxelTypeRegistry>();

	Log.log(LOG_INFO, "Gathering block types...");
	auto blockList = engine::Engine::Get()->getAssetManager()->getAssetList<asset::BlockType>();
	Log.log(LOG_INFO, "Found %i block types", blockList.size());
	this->mpVoxelTypeRegistry->registerEntries(blockList);
}

void World::createWorld()
{
	this->mpSceneOverworld = std::make_shared<physics::Scene>();
	this->mpSceneOverworld->setSystem(this->mpPhysics);
	this->mpSceneOverworld->create();

	this->mpChunkCollisionManager = std::make_shared<physics::ChunkCollisionManager>(
		this->mpPhysics, this->mpSceneOverworld
	);

	this->mpWorld = std::make_shared<world::Terrain>(this->mSaveData.seed());
	//if (this->mpVoxelInstanceBuffer) this->mpWorld->addEventListener(this->mpVoxelInstanceBuffer);
	this->mpWorld->addEventListener(this->mpChunkCollisionManager);

	this->mpPlayerPhysicsMaterial = std::make_shared<physics::Material>();
	this->mpPlayerPhysicsMaterial->setSystem(this->mpPhysics);
	this->mpPlayerPhysicsMaterial->create();

	this->createScene();
	//this->createLocalPlayer();

	// Specifically for clients which set player movement/camera information
	/*
	{
		auto pEngine = engine::Engine::Get();
		this->mpSystemMovePlayerByInput = pEngine->getMainMemory()->make_shared<ecs::system::MovePlayerByInput>();
		pEngine->addTicker(this->mpSystemMovePlayerByInput);
	}
	//*/

}

void World::destroyWorld()
{
	this->destroyScene();
	this->mpPlayerPhysicsMaterial.reset();
	this->mpChunkCollisionManager.reset();
	this->mpSceneOverworld.reset();
	this->mpWorld.reset();
}

void World::createScene()
{
	//this->mpWorld->loadChunk({ 0, 0, 0 });
	//this->mpWorld->loadChunk({ 0, 0, -1 });
	//for (i32 x = -1; x <= 1; ++x) for (i32 z = -1; z <= 1; ++z)
	//	this->mpWorld->loadChunk({ x, 0, z });
}

void World::destroyScene()
{
	//this->mpSystemMovePlayerByInput.reset();
	//this->mpEntityLocalPlayer.reset();
}

void World::update(f32 deltaTime)
{
	this->mpSceneOverworld->simulate(deltaTime);
}
