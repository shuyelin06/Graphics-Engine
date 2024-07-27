#include "Quaternion.h"

#include <math.h>

namespace Engine
{
namespace Math
{
	Quaternion::Quaternion() : im(), r(0) {}

	// Vector Constructor:
	// Creates a quaternion with given imaginary vector
	// and real value component
	Quaternion::Quaternion(const Vector3& _im, float _r)
	{
		im = _im;
		r = _r;
	}

	// Norm:
	// Calculates and returns the conjugate's norm
	float Quaternion::norm() const
	{
		float sq_prod = im.x * im.x + im.y * im.y + im.z * im.z + r * r;
		return sqrtf(sq_prod);
	}

	// Conjugate:
	// Returns this quaternion's conjugate, the quaternion such that
	// its product with this gives us a real number
	Quaternion Quaternion::conjugate() const
	{
		Quaternion new_qhat;
		new_qhat.im = -im;
		new_qhat.r = r;
		return new_qhat;
	}

	/* Quaternion Operations */
	// Addition:
	// Adds two quaternions together and returns a new quaternion
	// representing their result
	Quaternion Quaternion::operator+(const Quaternion& qhat) const
	{
		Quaternion new_qhat;
		new_qhat.im = im + qhat.im;
		new_qhat.r = r + qhat.r;
		return new_qhat;
	}

	// Compound (In-Place) Addition
	// Adds one quaternion to the other, where this quaternion is
	// modified in-place
	Quaternion& Quaternion::operator+=(const Quaternion& qhat)
	{
		im += qhat.im;
		r += qhat.r;
		return *this;
	}

	// Multiplication
	// Takes the product of two quaternions, and returns 
	// the result of this product as a new quaternion
	Quaternion Quaternion::operator*(const Quaternion& qhat) const
	{
		Quaternion new_qhat;
		new_qhat.im = Vector3::CrossProduct(im, qhat.im) + qhat.im * r + im * qhat.r;
		new_qhat.r = r * qhat.r - Vector3::DotProduct(im, qhat.im);
		return new_qhat;
	}

	// Compound (In-Place) Multiplication
	// Multiplies a quaternion to this, modifying this quaternion
	// in place
	Quaternion& Quaternion::operator*=(const Quaternion& qhat)
	{
		const Vector3 imNew = Vector3::CrossProduct(im, qhat.im) + qhat.im * r + im * qhat.r;
		const float rNew = r * qhat.r - Vector3::DotProduct(im, qhat.im);
		im = imNew;
		r = rNew;
		return *this;
	}

	// Identity:
	// Returns the identity quaternion, with a 0 imaginary vector
	// and real component equal to 1
	Quaternion Quaternion::Identity()
	{
		return Quaternion(Vector3(0, 0, 0), 1);
	}

	// RotationAroundAxis:
	// Generate a unit quaternion representing a rotation around a given axis
	Quaternion Quaternion::RotationAroundAxis(const Vector3& axis, float theta)
	{
		return Quaternion(axis * sinf(theta), cosf(theta));
	}
}
}