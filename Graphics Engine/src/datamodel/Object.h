#pragma once

#include "Transform.h"

#include "math/Vector3.h"
#include "math/Matrix4.h"

#include "input/InputData.h"
#include "rendering/Mesh.h"

#include "simulation/Dynamics.h"

namespace Engine
{
using namespace Math;
using namespace Graphics;
using namespace Simulation;

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
		std::vector<Object*> children;

		// Transform of the object
		Transform transform;
		
		// Renderable Attributes
		Mesh* mesh;

		// Physics Attributes
		Dynamics* dynamics;

		// Physics Attributes
		// May be dropped later
		Vector3 velocity;
		Vector3 acceleration;

	public:
		// Constructor
		Object();

		// Destructor
		~Object();

		// Update the Object
		// Can be overriden for object-specific behaviors
		void update();

		// Accessors
		Object* getParent() const;
		std::vector<Object*>& getChildren();
		Mesh* getMesh() const;

		Transform& getTransform();
		Vector3& getVelocity();
		Vector3& getAcceleration();

		// Setters
		void setParent(Object* parent);
		void setMesh(Mesh* mesh);

		// Create Child
		Object& createChild();

		// Local -> World Transform Matrix
		Matrix4 localToWorldMatrix() const;
	};
}
}