#pragma once

namespace Engine
{
namespace Physics
{
	// Class PhysicsEngine:
	// Handles all of the physics that occur in the engine
	class PhysicsEngine
	{
	public:
		PhysicsEngine();

		// Update the physics in a scene
		void update();
	};
}
}