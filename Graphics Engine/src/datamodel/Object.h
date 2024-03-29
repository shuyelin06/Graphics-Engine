#pragma once

#include "Transform.h"

#include "math/Vector3.h"
#include "math/Matrix4.h"

#include "rendering/Mesh.h"

namespace Engine::Graphics { class VisualAttribute; }

namespace Engine
{
using namespace Math;
using namespace Graphics;
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
		// Object Transformation Fields
		Object* parent;			// Reference to parent

		// Transform of the object
		Transform transform;
		
		// (TBD) - Renderable Attribute
		VisualAttribute* visual_attr;

	public:
		Object();

		// Get object transform. Only possible with a transform accessor.
		Transform* getTransform(ObjectAccessor);
		Object* getParent(ObjectAccessor);

		// Get Transform
		Transform* getTransform();

		// Set Parent
		void setVisualAttribute(VisualAttribute* attribute);
		void setParent(Object* parent); 
	};


}
}