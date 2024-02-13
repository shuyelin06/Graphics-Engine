#pragma once

#include "Object.h"

namespace Engine
{

namespace Datamodel
{
	// Class PhysicsObject:
	// Implements an object that can interact with
	// other objects with physics
	class PhysicsObject : public Object
	{
	protected:
		Vector3 acceleration;
		Vector3 velocity;
		
	public:
		PhysicsObject();

		void physicsUpdate(float delta_time);

		// Set acceleration
		void setAcceleration(float x, float y, float z);
		// Offset acceleration
		void offsetAcceleration(float x, float y, float z);
		void offsetAcceleration(Vector3 offset);

		// Set velocity 
		void setVelocity(float x, float y, float z);
		void setVelocity(Vector3 velocity);
		// Offset velocity
		void offsetVelocity(float x, float y, float z);
		void offsetVelocity(Vector3 offset);

	};

}
}