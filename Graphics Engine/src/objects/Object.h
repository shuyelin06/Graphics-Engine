#pragma once

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
		Mesh* mesh;				// Reference to a mesh for rendering

		Object* parent;			// pointer to object's parent
		Vector3 rotation;		// roll, yaw, pitch
		Vector3 scale;			// scaleX, scaleY, scaleZ
		Vector3 position_local; // x, y, z

	public:
		Object();

		// Set Parent
		void setParent(Object* parent);

		// Determine distance to another object
		float distanceTo(Object* o);

		// Get (copy of) local position
		Vector3 getPosition(); 
		
		// Get (copy of) world position
		Vector3 getWorldPosition();

		// Transformations on the object
		void setPosition(float x, float y, float z);
		void setPosition(Vector3 position);
		void offsetPosition(float offsetX, float offsetY, float offsetZ);
		void offsetPosition(Vector3 offset);

		void setScale(float scaleX, float scaleY, float scaleZ);
		void offsetScale(float offsetX, float offsetY, float offsetZ);

		void setRotation(float roll, float pitch, float yaw);
		void offsetRotation(float rollDelta, float pitchDelta, float yawDelta);

		Matrix4 localToWorldMatrix(void);

		// Get and set object's renderable mesh
		void setMesh(Mesh* mesh);
		Mesh* getMesh();

		Matrix4 rotationMatrix();

	protected:
		// Helper methods to produce transformation matrices
		Matrix4 scaleMatrix();
		
		Matrix4 translationMatrix();
	};
}
}