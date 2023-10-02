#pragma once

namespace Engine
{
namespace Math
{

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

		Vector4 operator+(const Vector4&) const;
		Vector4 operator-(const Vector4&) const;
		Vector4 operator*(const Vector4&) const;
		Vector4 operator/(const Vector4&) const;
	};

} // Namespace Math
} // Namespace Engine