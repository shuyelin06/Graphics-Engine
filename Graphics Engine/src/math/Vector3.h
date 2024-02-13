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

		void normalize(); // In-place normalize

		float magnitude() const;

		Vector3 operator+(const Vector3&) const;
		Vector3 operator-(const Vector3&) const;
		Vector3 operator-() const;
		Vector3 operator*(const float) const;
		Vector3 operator/(const float) const;

		static Vector3 PositiveX();
		static Vector3 PositiveY();
		static Vector3 PositiveZ();
	};


} // Namespace Math
} // Namespace Engine