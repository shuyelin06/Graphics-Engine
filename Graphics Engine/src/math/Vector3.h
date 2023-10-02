#pragma once

namespace Engine
{
namespace Math
{

	// Vector3
	// Contains methods and data for a 3-dimensional
	// vector.
	class Vector3
	{
	public:
		float x, y, z;

		Vector3();
		Vector3(const Vector3& copy);
		Vector3(float _x, float _y, float _z);

		~Vector3() {};

		void normalize();

		float magnitude() const;

		Vector3 operator+(const Vector3&) const;
		Vector3 operator-(const Vector3&) const;
		Vector3 operator*(const float) const;
		Vector3 operator/(const float) const;
	};


} // Namespace Math
} // Namespace Engine