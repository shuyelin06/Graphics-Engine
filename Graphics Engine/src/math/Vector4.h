#pragma once

#include "Vector3.h"

namespace Engine
{
namespace Math
{
	class Matrix4;

	// Vector4
	// Contains methods and data for a 4-dimensional
	// vector.
	class Vector4
	{
	public:
		float x, y, z, w;

		Vector4();
		Vector4(const Vector4& copy);
		Vector4(float _x, float _y, float _z, float _w);

		~Vector4();

		void normalize();

		float magnitude();
		Vector3 toVector3(); // Drops the w term

		Vector4 operator*(Matrix4&) const; // Row-Major Multiplication
		Vector4 operator+(const Vector4&) const;
		Vector4 operator-(const Vector4&) const;
		Vector4 operator*(const Vector4&) const;
		Vector4 operator/(const Vector4&) const;
		
		static Vector4 PositiveXW();
		static Vector4 PositiveYW();
		static Vector4 PositiveZW();
	};

} // Namespace Math
} // Namespace Engine