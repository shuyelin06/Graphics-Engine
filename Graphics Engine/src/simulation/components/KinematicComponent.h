#pragma once

#include "datamodel/Component.h"

#include "math/Vector3.h"

namespace Engine
{
namespace Simulation
{
	// Forward Declaration of PhysicsSystem
	class PhysicsSystem;

	// KinematicComponent Class:
	// Enables object motion, with regard to velocity,
	// acceleration, and other ideas in classical mechanics.
	// All vectors are expected to be in relation to the object's center
	// at (in local space) (0,0,0).
	// This component must be associated with a PhysicsSystem, and should only be
	// created from a PhysicsSystem.
	class KinematicComponent : Datamodel::Component
	{
	private:
		Math::Vector3 velocity;
		Math::Vector3 acceleration;

	public:
		KinematicComponent(Datamodel::Object* object, PhysicsSystem* system);
		
		void update(float delta_time);
	};
}
}