#pragma once

#include "datamodel/Component.h"

namespace Engine
{
namespace Input
{
	// Forward Declare of InputSystem
	class InputSystem;

	// MovementComponent Class:
	// Implements basic WASD movement and mouse rotations for an object,
	// by polling the input system.
	class MovementComponent : 
		public Datamodel::Component<MovementComponent>
	{
	private:
		// Center of the screen
		int center_x, center_y;

		float sensitivity;

	public:
		MovementComponent(Datamodel::ComponentHandler<MovementComponent>* handler);
		
		// Polls the input system to update the target
		// object's transform
		void update();
	};
}
}