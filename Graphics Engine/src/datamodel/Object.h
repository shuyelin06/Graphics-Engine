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
	// Every object has a parent and children. Together, objects form a scene.
	class Object
	{
	protected:
		// Parent & Children
		Object* parent;
		vector<Object*> children;

		// Transform of the object
		Transform transform;
		
		// Renderable Mesh
		Mesh* mesh;

		// Physics Attributes
		// May be dropped later
		Vector3 velocity;
		Vector3 acceleration;

	public:
		// Constructor
		Object();

		// Destructor
		~Object();

		// Accessors
		Object* getParent() const;
		vector<Object*>& getChildren();
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