#include "PhysicsSystem.h"

namespace Engine
{
using namespace Utility;

namespace Simulation
{
	// Constructor:
	// Initializes relevant fields
	PhysicsSystem::PhysicsSystem()
		: stopwatch()
	{

	}
	
	// Initialize:
	// Begin the stopwatch to track delta_time.
	void PhysicsSystem::initialize()
	{
		stopwatch.Reset();
	}

	// Update:
	// Updates the physics for a scene. 
	void PhysicsSystem::update()
	{
		// Receive delta time and reset stopwatch
		float delta_time = stopwatch.Duration();
		stopwatch.Reset();

		// Execute all kinematic components
		// for (KinematicComponent* kin_comp : ComponentHandler<KinematicComponent>::components)
		//	kin_comp->update(delta_time);
	}

}
}