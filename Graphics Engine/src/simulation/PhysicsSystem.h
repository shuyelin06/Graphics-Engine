#pragma once

#include <vector>

#include "datamodel/Subsystem.h"
#include "datamodel/ComponentHandler.h"

#include "utility/Stopwatch.h"

#include "components/KinematicComponent.h"

namespace Engine
{
namespace Simulation
{
	// PhysicsSystem Class
	// Manages physics behaviors in the game engine.
	class PhysicsSystem : 
		public Datamodel::Subsystem, 
		public Datamodel::ComponentHandler<KinematicComponent>
	{
	private:
		// Track delta time
		Utility::Stopwatch stopwatch;

	public:
		PhysicsSystem();

		// Performs relevant initializations for the scene physics
		void initialize();

		// Updates the physics for a scene
		void update();
	};
}
}