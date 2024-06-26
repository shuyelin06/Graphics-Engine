#include "MovementComponent.h"

#include "input/InputSystem.h"
#include "input/callbacks/InputPoller.h"

namespace Engine
{
namespace Input
{
	MovementComponent::MovementComponent(Datamodel::ComponentHandler<MovementComponent>* handler)
		: Datamodel::Component<MovementComponent>(handler)
	{
		sensitivity = 1;

		center_x = 600;
		center_y = 600;
	}

	void MovementComponent::update()
	{
		Datamodel::Transform& transform = object->getTransform();

		// Handle XYZ movement first. 
		Vector3 movementVector = Vector3();

		// Poll the input system for the status of the WASDQE keys.
		// Use this to form a movement vector indicating the direction
		// to move in. 
		if (InputPoller::IsSymbolActive('w'))
			movementVector += transform.forwardVector();
		if (InputPoller::IsSymbolActive('s'))
			movementVector += transform.backwardVector();
		if (InputPoller::IsSymbolActive('a'))
			movementVector += transform.leftVector();
		if (InputPoller::IsSymbolActive('d'))
			movementVector += transform.rightVector();
		if (InputPoller::IsSymbolActive('q'))
			movementVector += transform.downVector();
		if (InputPoller::IsSymbolActive('e'))
			movementVector += transform.upVector();

		// If any movement input is active, offset the object
		// position.
		// Normalize movement vector and offset position
		if (movementVector.magnitude() != 0)
		{
			movementVector.inplaceNormalize();

			transform.offsetPosition(movementVector.x, movementVector.y, movementVector.z);
		}
		
		// Then, handle camera rotation movement.
		POINT new_pos;
		GetCursorPos(&new_pos);

		int x_delta = new_pos.x - center_x;
		int y_delta = new_pos.y - center_y;

		// Convert to Angular Displacement
		// Roll = Rotation Around X (Up/Down)
		// Pitch = Rotation Around Y (Left/Right
		float roll_delta = y_delta / 100.f;
		float pitch_delta = x_delta / 100.f;

		object->getTransform().offsetRotation(roll_delta, pitch_delta, 0);

		// Reset mouse to center of application
		SetCursorPos(center_x, center_y);
	}
}
}