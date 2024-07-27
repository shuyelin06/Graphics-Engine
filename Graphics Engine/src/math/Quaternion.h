#pragma once

#include "Vector3.h"

namespace Engine
{
namespace Math
{
	// Quaternion Class:
	// Represents a quaternion, which can be used to represent rotations
	// in 3D space
	// Quaternions are given in the form
	// xi + yj + zk + r = q
	// Where i,j,k are imaginary components. 
	// If we express quaternions in the form 
	// (sin(theta) * axis, cos(theta)), we can use them
	// to represent a rotation around the axis in space.
	class Quaternion
	{
	public:
		// Imaginary Component
		Vector3 im;
		
		// Real component
		float r;

	public:
		Quaternion();
		Quaternion(const Vector3& im, float real);

		float norm() const;
		Quaternion conjugate() const;

		// Quaternion Operations
		Quaternion operator+(const Quaternion&) const; // Addition
		Quaternion& operator+=(const Quaternion&);	   // Compound (In-Place) Addition
		Quaternion operator*(const Quaternion&) const; // Product
		Quaternion& operator*=(const Quaternion&);     // Compound (In-Place) Product

		// Identity Quaternion
		static Quaternion Identity();

		// Generate a unit quaternion representing a rotation around a given axis
		static Quaternion RotationAroundAxis(const Vector3& axis, float theta);
	};
}
}