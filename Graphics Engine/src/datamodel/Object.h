#pragma once

#include "Transform.h"

#include "math/Vector3.h"
#include "math/Matrix4.h"

#include "rendering/VisualAttribute.h"
#include "rendering/Mesh.h"

namespace Engine
{
using namespace Math;
using namespace Graphics;
namespace Datamodel
{

	// Object
	// Stores data regarding a generic object in our engine. 
	class Object
	{
	protected:
		// Object Transformation Fields
		Object* parent;			// Reference to parent

		// Transform of the object
		Transform transform;
		
		// Object Renderable Fields
		Mesh* mesh;				// Reference to renderable mesh

		// (TBD) - Renderable Attribute
		VisualAttribute* visual_attr;

		// Object Physics Fields
		Vector3 velocity;
		Vector3 acceleration;

	public:
		Object();

		// Get Transform
		Transform* getTransform();

		// Set Parent
		void setParent(Object* parent); 

		// Set Renderable Mesh
		void setMesh(Mesh* mesh);

	// Accessible by the VisualEngine Class for Rendering
	friend void Engine::Graphics::VisualAttribute::prepare();
	friend class Engine::Graphics::VisualEngine;
	};
}
}