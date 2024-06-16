#pragma once

#include "datamodel/Scene.h"

namespace Engine
{
using namespace Datamodel;
namespace Simulation
{
	// Class PhysicsEngine:
	// Handles all of the physics that occur in the engine
	class SimulationEngine
	{
	public:
		SimulationEngine();

		// Initialize Simulation Engine
		void initalize();

		// Update the physics of an entire scene
		void update(Scene& scene);
	};
}
}