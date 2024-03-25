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
		Vector3 position_local; // x, y, z
		Vector3 rotation;		// roll, yaw, pitch
		Vector3 scale;			// scaleX, scaleY, scaleZ
		
		// Object Renderable Fields
		Mesh* mesh;				// Reference to renderable mesh

		// (TBD) - Renderable Attribute
		VisualAttribute* visual_attr;

		// Object Physics Fields
		Vector3 velocity;
		Vector3 acceleration;

	public:
		Object();

		// Set Parent
		void setParent(Object* parent); 

		// Set Renderable Mesh
		void setMesh(Mesh* mesh);		

		// Set and Offset Object Position
		void setPosition(float x, float y, float z);
		void setPosition(Vector3 position);

		// Set and Offset Object Position
		void offsetPosition(float offsetX, float offsetY, float offsetZ);
		void offsetPosition(Vector3 offset);

		// Set and Offset Scale
		void setScale(float scaleX, float scaleY, float scaleZ);
		void offsetScale(float offsetX, float offsetY, float offsetZ);

		// Set and Offset Rotation
		void setRotation(float roll, float pitch, float yaw);
		void offsetRotation(float rollDelta, float pitchDelta, float yawDelta);

		// Determine distance to another object
		float distanceTo(Object* o);

		// Get (copy of) local position
		Vector3 getPosition(); 
		
		// Get (copy of) world position
		Vector3 getWorldPosition();

	protected:
		// Private Methods that Create Matrices for Rendering 
		Matrix4 localToWorldMatrix(void) const;
		
		Matrix4 scaleMatrix() const;
		Matrix4 rotationMatrix() const;
		Matrix4 translationMatrix() const;

	// Accessible by the VisualEngine Class for Rendering
	friend void Engine::Graphics::VisualAttribute::prepare();
	friend class Engine::Graphics::VisualEngine;
	};
}
}