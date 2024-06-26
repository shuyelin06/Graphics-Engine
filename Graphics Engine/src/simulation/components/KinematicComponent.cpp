#include "KinematicComponent.h"

#include "simulation/PhysicsSystem.h"

namespace Engine
{
using namespace Datamodel;

namespace Simulation
{
	// Constructor:
	// Registers the kinematic component with the
	// engine's PhysicsSystem, and initializes its
	// fields to 0.
	KinematicComponent::KinematicComponent(Datamodel::ComponentHandler<KinematicComponent>* _handler) : 
		Component<KinematicComponent>(handler),
		velocity(), acceleration()
	{

	}

	// Update:
	// Updates the object's transform given the current velocity
	// and acceleration
	void KinematicComponent::update(float delta_time)
	{
		// Update velocity
		velocity += acceleration * delta_time;

		// Update object position
		Transform& transform = object->getTransform();
		transform.offsetPosition(velocity.x * delta_time, velocity.y * delta_time, velocity.z * delta_time);
	}
}
}