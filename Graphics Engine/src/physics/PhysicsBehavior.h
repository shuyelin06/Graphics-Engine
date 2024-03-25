#pragma once

// Forward Declaration of Object
namespace Engine::Datamodel { class Object; }

namespace Engine
{
namespace Physics
{
	// Class PhysicsBehavior:
	// Defines the physical behavior for a given
	// object 
	class PhysicsBehavior
	{
	protected:
		
		// Prepare the object for physics
		virtual void prepare();

		//
		virtual void update();
		
	};
}
}