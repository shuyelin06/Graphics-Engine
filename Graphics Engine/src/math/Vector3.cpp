#include "Vector3.h"

#include <math.h>

namespace Engine
{
namespace Math
{

	// Default Constructor:
	// Creates a vector whose x,y,z values are all 0
	Vector3::Vector3()
	{
		x = y = z = 0.f;
	}

	// Copy Constructor:
	// Creates a vector which is a copy of the 
	// vector passed in 
	Vector3::Vector3(const Vector3& copy)
	{
		x = copy.x;
		y = copy.y;
		z = copy.z;
	}

	// Constructor:
	// Creates a vector given input x,y,z values
	Vector3::Vector3(float _x, float _y, float _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}

	// Magnitude:
	// Returns the vector's magnitude
	float Vector3::magnitude() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	// InplaceNormalize:
	// Normalizes the vector inplace
	void Vector3::inplaceNormalize()
	{
		float magn = magnitude();
		x /= magn;
		y /= magn;
		z /= magn;
	}

	// + Operator:
	// Returns a new vector which is the summation of
	// this vector and another vector
	Vector3 Vector3::operator+(const Vector3& vec) const
	{
		Vector3 vector;
		vector.x = x + vec.x;
		vector.y = y + vec.y;
		vector.z = z + vec.z;
		return vector;
	}

	// += Operator
	// Adds a vector's x,y,z values to this vector
	// in-place
	Vector3& Vector3::operator+=(const Vector3& vec)
	{
		x += vec.x;
		y += vec.y;
		z += vec.z;
		return *this;
	}

	// - Operator
	// Returns a new vector which is the result of this vector
	// subtracted with another vector
	Vector3 Vector3::operator-(const Vector3& vec) const
	{
		return Vector3(
			x - vec.x,
			y - vec.y,
			z - vec.z
		);
	}

	// -= Operator
	// Subtracts another vector's x,y,z values from this vector
	// in-place
	Vector3& Vector3::operator-=(const Vector3& vec)
	{
		x -= vec.x;
		y -= vec.y;
		z -= vec.z;
		return *this;
	}

	// - Operator (Negate)
	// Returns a new vector which is the negation of this vector's
	// x,y,z values
	Vector3 Vector3::operator-() const
	{
		return Vector3(-x, -y, -z);
	}

	// * Operator
	// Returns a new vector which is the result of this vector
	// scalar multiplied by some value
	Vector3 Vector3::operator*(const float f) const
	{
		Vector3 vector;
		vector.x = x * f;
		vector.y = y * f;
		vector.z = z * f;
		return vector;
	}

	// *= Operator
	// Multiplies this vector's values by a scalar
	// value in-place
	Vector3& Vector3::operator*=(const float f)
	{
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	// / Operator
	// Returns a new vector which is the result of this vector
	// divided by a scalar value
	Vector3 Vector3::operator/(const float f) const
	{
		Vector3 vector;
		vector.x = x / f;
		vector.y = y / f;
		vector.z = z / f;
		return vector;
	}

	// /= Operator
	// Divides this vector's values by a scalar value in-place
	// Assumes f is not 0.
	Vector3& Vector3::operator/=(const float f)
	{
		x /= f;
		y /= f;
		z /= f;
		return *this;
	}

	// DotProduct:
	// Calculates the dot product between two vectors.
	float Vector3::DotProduct(const Vector3& v1, const Vector3& v2)
	{
		float result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
		return result;
	}

	// CrossProduct:
	// Calculates the cross product between two vectors. Note that
	// the cross product is sensitive to the order of vectors
	// provided.
	Vector3 Vector3::CrossProduct(const Vector3& v1, const Vector3& v2)
	{
		Vector3 vector;
		vector.x = v1.y * v2.z - v1.z * v2.y;
		vector.y = v1.z * v2.x - v1.x * v2.z;
		vector.z = v1.x * v2.y - v1.y * v2.x;
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