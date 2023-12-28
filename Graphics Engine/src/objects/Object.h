#pragma once

#include "math/Vector3.h"
#include "math/Matrix4.h"

#include "rendering/buffers/Vertexbuffer.h"

namespace Engine
{
using namespace Math;
using namespace Graphics;

namespace Datamodel
{

	// Object
	// Stores data regarding a generic
	// object in our engine. 
	class Object
	{
	private:
		VertexBuffer vertex_buffer; // vertex buffer (for rendering)

		Object* parent;			// pointer to object's parent
		Vector3 rotation;		// roll, yaw, pitch
		Vector3 scale;			// scaleX, scaleY, scaleZ
		Vector3 position_local; // x, y, z

	public:
		Object();

		// Transformations on the object
		void setPosition(float x, float y, float z);
		void offsetPosition(float offsetX, float offsetY, float offsetZ);

		void setScale(float scaleX, float scaleY, float scaleZ);
		void offsetScale(float offsetX, float offsetY, float offsetZ);

		void setRotation(float roll, float yaw, float pitch);
		void offsetRotation(float offsetX, float offsetY, float offsetZ);

		Matrix4 localToWorldMatrix(void);

		// Rendering for the object
		void setVertexBuffer(VertexBuffer buffer);
		VertexBuffer getVertexBuffer(void);

	private:
		// Helper methods to produce transformation matrices
		Matrix4 scaleMatrix();
		Matrix4 rotationMatrix();
		Matrix4 translationMatrix();
	};
}
}