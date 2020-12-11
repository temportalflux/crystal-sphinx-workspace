#pragma once

#include "ecs/system/System.hpp"

NS_ECS
NS_SYSTEM

class PhysicsIntegration : public System
{
public:
	PhysicsIntegration();
	void update(f32 deltaTime, std::shared_ptr<ecs::view::View> view) override;
};

NS_END
NS_END
