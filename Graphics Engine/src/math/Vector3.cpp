#include "Vector3.h"

#include <math.h>

namespace Engine
{
namespace Math
{

	Vector3::Vector3()
	{
		x = y = z = 0.f;
	}

	Vector3::Vector3(const Vector3& copy)
	{
		x = copy.x;
		y = copy.x;
		z = copy.z;
	}

	Vector3::Vector3(float _x, float _y, float _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}

	float Vector3::magnitude() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	void Vector3::normalize()
	{
		float length = magnitude();
		x /= length;
		y /= length;
		z /= length;
	}

	Vector3 Vector3::operator+(const Vector3& vec) const
	{
		Vector3 vector;
		vector.x = x + vec.x;
		vector.y = y + vec.y;
		vector.z = z + vec.z;
		return vector;
	}

	Vector3 Vector3::operator-(const Vector3& vec) const
	{
		return Vector3(
			x - vec.x,
			y - vec.y,
			z - vec.z
		);
	}

	Vector3 Vector3::operator-() const
	{
		return Vector3(-x, -y, -z);
	}

	Vector3 Vector3::operator*(const float f) const
	{
		Vector3 vector;
		vector.x = x * f;
		vector.y = y * f;
		vector.z = z * f;
		return vector;
	}

	Vector3 Vector3::operator/(const float f) const
	{
		Vector3 vector;
		vector.x = x / f;
		vector.y = y / f;
		vector.z = z / f;
		return vector;
	}

	Vector3 Vector3::PositiveX()
	{
		return Vector3(1, 0, 0);
	}

	Vector3 Vector3::PositiveY()
	{
		return Vector3(0, 1, 0);
	}

	Vector3 Vector3::PositiveZ()
	{
		return Vector3(0, 0, 1);
	}

} // Namespace Math
} // Namespace Engine