#pragma once

#include "Transform.h"

#include "math/Vector3.h"
#include "math/Matrix4.h"

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
		// Transform of the object
		Transform transform;
		
		// Physics Attributes
		Vector3 velocity;
		Vector3 acceleration;

		// Parent
		Object* parent;

		// Renderable Mesh
		Mesh* mesh;

	public:
		// Constructor
		Object();

		// Accessors
		Object* getParent() const;
		Mesh* getMesh() const;

		Transform& getTransform();
		Vector3& getVelocity();
		Vector3& getAcceleration();

		// Setters
		void setParent(Object* parent);
		void setMesh(Mesh* mesh);

		// Local -> World Transform Matrix
		Matrix4 localToWorldMatrix() const;
	};
}
}