#pragma once

#include "Vector3.h"

namespace Engine
{
namespace Math
{
	// Class Quaternion
	// Represents a quaternion, which often finds
	// great practical use in rotations
	class Quaternion
	{
	public:
		// Imaginary component
		Vector3 qv;
		
		// Real component
		float qw;

	public:
		Quaternion();
		Quaternion(const Vector3&, float theta);

		float norm() const;
		Quaternion conjugate() const;

		// Quaternion Operations
		Quaternion operator+(const Quaternion&) const; // Addition
		Quaternion& operator+=(const Quaternion&);	   // Compound (In-Place) Addition
		Quaternion operator*(const Quaternion&) const; // Product
		Quaternion& operator*=(const Quaternion&);     // Compound (In-Place) Product

		// Identity Quaternion
		static Quaternion Identity();
	};
}
}