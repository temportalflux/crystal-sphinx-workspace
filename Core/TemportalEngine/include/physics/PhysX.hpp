#pragma once

#include "TemportalEnginePCH.hpp"

#if !defined(_DEBUG)
#define PHYSX_DEBUG_RESET 
#endif

#if !defined(NDEBUG) && !defined(_DEBUG)
#define _DEBUG
#endif
#include "PxPhysicsAPI.h"
#include "extensions/PxDefaultAllocator.h"
#include "characterkinematic/PxControllerManager.h"
#if defined(PHYSX_DEBUG_RESET)
#undef _DEBUG
#endif

#define TE_PHYSX_VERSION TE_MAKE_VERSION(PX_PHYSICS_VERSION_MAJOR, PX_PHYSICS_VERSION_MINOR, PX_PHYSICS_VERSION_BUGFIX)

FORWARD_DEF(NS_LOGGING, class Logger);

NS_PHYSICS

class ErrorCallback : public physx::PxErrorCallback
{
	logging::Logger *mpLogger;
public:
	ErrorCallback();
	void init(logging::Logger *pLogger);
	void invalidate();
	void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override;
};

physx::PxVec3 toPhysX(math::Vector3 const& v);
physx::PxExtendedVec3 toPhysX(math::Vector<f64, 3> const& v);
math::Vector<f64, 3> fromPhysX(physx::PxExtendedVec3 const& v);
physx::PxQuat toPhysX(math::Quaternion const& v);

NS_END