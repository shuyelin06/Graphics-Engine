#include "PhysicsObject.h"

namespace Engine
{

namespace Datamodel
{
	// Constructor:
	// Initialize acceleration and velocity as 0
	PhysicsObject::PhysicsObject()
	{
		velocity = Vector3(0, 0, 0);
		acceleration = Vector3(0, 0, 0);
	}

	// PhysicsUpdate:
	// Update the object's physics
	void PhysicsObject::physicsUpdate(float delta_time)
	{
		// Update velocity with acceleration
		offsetVelocity(acceleration * delta_time);

		// Update position with velocity
		offsetPosition(velocity * delta_time);
	}

	// SetAcceleration:
	// Sets the object's acceleration
	void PhysicsObject::setAcceleration(float x, float y, float z)
	{
		acceleration.x = x;
		acceleration.y = y;
		acceleration.z = z;
	}

	// OffsetAcceleration:
	// Offsets the object's acceleration
	void PhysicsObject::offsetAcceleration(float x, float y, float z)
	{
		setAcceleration(acceleration.x + x, acceleration.y + y, acceleration.z + z);
	}

	void PhysicsObject::offsetAcceleration(Vector3 offset)
	{
		offsetAcceleration(offset.x, offset.y, offset.z);
	}

	// SetVelocity:
	// Sets the object's velocity
	void PhysicsObject::setVelocity(float x, float y, float z)
	{
		velocity.x = x;
		velocity.y = y;
		velocity.z = z;
	}

	// OffsetVelocity:
	// Offsets the obejct's velocity
	void PhysicsObject::offsetVelocity(float x, float y, float z)
	{
		setVelocity(velocity.x + x, velocity.y + y, velocity.z + z);
	}

	// OffsetVelocity:
	// Offsets the obejct's velocity
	void PhysicsObject::offsetVelocity(Vector3 offset)
	{
		offsetVelocity(offset.x, offset.y, offset.z);
	}


}
}