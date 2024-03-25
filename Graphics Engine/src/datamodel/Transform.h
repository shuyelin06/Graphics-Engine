#pragma once

#include "math/Matrix4.h"
#include "math/Vector3.h"

namespace Engine
{
using namespace Math;
namespace Datamodel
{
	// Class Transform:
	// Contains the data and methods regarding
	// an object's transform
	class Transform 
	{
	private:
		Vector3 position_local; // X,Y,Z Local Position
		Vector3 rotation;		// Roll, Yaw, Pitch Rotation
		Vector3 scale;			// ScaleX, ScaleY, ScaleZ

	public:
		// Constructor
		Transform();

		// Get and set the transform properties
		const Vector3 getPosition() const;
		void setPosition(float x, float y, float z);
		void offsetPosition(float x, float y, float z);

		const Vector3 getRotation() const;
		void setRotation(float roll, float yaw, float pitch);
		void offsetRotation(float roll, float yaw, float pitch);

		const Vector3 getScale() const;
		void setScale(float x, float y, float z);
		void offsetScale(float x, float y, float z);

		// Generates transformation matrices based off transform
		Matrix4 transformMatrix(void) const;

		Matrix4 scaleMatrix(void) const;
		Matrix4 rotationMatrix(void) const;
		Matrix4 translationMatrix(void) const;
	};
}
}