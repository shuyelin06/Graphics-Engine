#pragma once

#include "Transform.h"

#include "math/Vector3.h"
#include "math/Matrix4.h"

namespace Engine::Graphics { class VisualAttribute; }

namespace Engine
{
using namespace Math;
namespace Datamodel
{

	// ObjectAccessor Class:
	// Allows access to an object's properties
	class ObjectAccessor
	{
		friend class Engine::Graphics::VisualAttribute;
	
	private:
		ObjectAccessor();
	};

	// Object Class:
	// Stores data regarding a generic object in our engine. 
	class Object
	{
	protected:
		// Transform of the object
		Transform transform;
		
		// Parent Attribute
		Object* parent;

		// Renderable Attribute
		Graphics::VisualAttribute* visual_attr;

	public:
		Object();

		// Get object transform. Only possible with a transform accessor.
		Transform* getTransform(ObjectAccessor);
		Object* getParent(ObjectAccessor);

		// Get Transform
		Transform* getTransform();

		// Set Attributes
		void setVisualAttribute(Graphics::VisualAttribute* attribute);
		void setParent(Object* parent); 
	};

}
}