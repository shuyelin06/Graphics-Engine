#pragma once

#include "math/Matrix4.h"

// Forward Declaration of Object
namespace Engine::Datamodel { class Object;  }

namespace Engine
{
using namespace Math;
using namespace Datamodel;
namespace Graphics
{
	// Class VisualAttribute:
	// Describes the rendering behavior for objects
	class VisualAttribute
	{
	protected:
		Object* object;
		Matrix4 local_to_world;

	public:
		VisualAttribute(Object* object);
		~VisualAttribute();

		// Prepares an object for rendering. Always
		// loads the object's local_to_world matrix first.
		virtual void prepare() = 0;
		
		// Renders an object 
		virtual void render() = 0;

		// Finish the rendering for an object
		virtual void finish() = 0;
	};
}
}