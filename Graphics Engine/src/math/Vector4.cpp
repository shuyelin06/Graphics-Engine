#include "Vector4.h"

#include <math.h>

namespace Engine
{
namespace Math
{

	/* --- Constructors --- */
	// Default Constructor:
	// Returns a Vector4 whose components are all 
	// initialized to 0
	Vector4::Vector4()
	{
		x = y = z = w = 0.f;
	}

	// Copy Constructor:
	// Given a Vector4, creates a copy of the vector
	// and returns it
	Vector4::Vector4(const Vector4& copy)
	{
		x = copy.x;
		y = copy.y;
		z = copy.z;
		w = copy.w;
	}

	// Constructor:
	// Creates a Vector4 with given initial values
	Vector4::Vector4(float _x, float _y, float _z, float _w)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}

	
	/* --- Destructor --- */
	Vector4::~Vector4() {}

	/* --- Operations --- */
	// Normalize:
	// Normalizes the vector (in place)
	void Vector4::normalize()
	{
		float length = magnitude();

		x /= length;
		y /= length;
		z /= length;
		w /= length;
	}

	// Magnitude:
	// Returns the magnitude of the vector
	float Vector4::magnitude()
	{
		return sqrtf(x * x + y * y + z * z + w * w);
	}

	// ToVector3:
	// Drops the w component of the vector
	Vector3 Vector4::toVector3()
	{
		return Vector3(x, y, z);
	}

	/* --- Operands --- */
	// Add:
	// Adds two Vector4's together
	Vector4 Vector4::operator+(const Vector4& vec) const
	{
		return Vector4(
			x + vec.x,
			y + vec.y,
			z + vec.z,
			w + vec.w);
	}

	// Subtract:
	// Subtracts one Vector4 from another
	Vector4 Vector4::operator-(const Vector4& vec) const
	{
		return Vector4(
			x - vec.x,
			y - vec.y,
			z - vec.z,
			w - vec.w);
	}

	// Multiply:
	// Multiplies two Vector4's together
	Vector4 Vector4::operator*(const Vector4& vec) const
	{
		return Vector4(
			x * vec.x,
			y * vec.y,
			z * vec.z,
			w * vec.w);
	}

	// Divide:
	// Divides one Vector4 from another
	Vector4 Vector4::operator/(const Vector4& vec) const
	{
		return Vector4(
			x / vec.x,
			y / vec.y,
			z / vec.z,
			w / vec.w);
	}

	// PositiveXW:
	// Returns the vector in the positive x
	// and w direction
	Vector4 Vector4::PositiveXW() 
	{
		return Vector4(1, 0, 0, 1);
	}

	// PositiveYW:
	// Returns the vector in the positive x
	// and w direction (w = 1)
	Vector4 Vector4::PositiveYW()
	{
		return Vector4(0, 1, 0, 1);
	}

	// PositiveZ:
	// Returns the vector in the positive x
	// and w direction (w = 1)
	Vector4 Vector4::PositiveZW()
	{
		return Vector4(0, 0, 1, 1);
	}

} // Namespace Math
} // Namespace Engine