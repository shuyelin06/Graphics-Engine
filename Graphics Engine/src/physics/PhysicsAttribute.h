#pragma once

#include <vector>

#include "datamodel/Object.h"

namespace Engine
{
using namespace Datamodel;
namespace Physics
{
	// Class PhysicsAttribute:
	// Provides an interface for physics in the application
	class PhysicsAttribute
	{
	protected:
		float delta_time;

	public:
		PhysicsAttribute(Object* object);
		~PhysicsAttribute();

		// Prepare for a physics update
		virtual void prepare(void) = 0;
		
		// Update object physics
		virtual void update(void) = 0;

		// Finish updating object physics
		virtual void finish(void) = 0;
	};
}
}