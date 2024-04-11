#include "Quaternion.h"

#include <math.h>

namespace Engine
{
namespace Math
{
	// Default Constructor:
	// Creates a 0-quaternion, with imaginary component 
	// equal to the 0-vector, and real component 0
	Quaternion::Quaternion()
	{
		qv = Vector3(0, 0, 0);
		qw = 0.f;
	}

	// Vector Constructor:
	// Creates a quaternion with given imaginary vector
	// and real value component
	Quaternion::Quaternion(const Vector3& _qv, float _qw)
	{
		qv = _qv;
		qw = _qw; 
	}

	// Norm:
	// Calculates and returns the conjugate's norm
	float Quaternion::norm() const
	{
		float sq_prod = qv.x * qv.x + qv.y * qv.y + qv.z * qv.z + qw * qw;
		return sqrtf(sq_prod);
	}

	// Conjugate:
	// Returns this quaternion's conjugate, the quaternion such that
	// its product with this gives us a real number
	Quaternion Quaternion::conjugate() const
	{
		Quaternion new_qhat;
		new_qhat.qv = -qv;
		new_qhat.qw = qw;
		return new_qhat;
	}

	/* Quaternion Operations */
	// Addition:
	// Adds two quaternions together and returns a new quaternion
	// representing their result
	Quaternion Quaternion::operator+(const Quaternion& qhat) const
	{
		Quaternion new_qhat;
		new_qhat.qv = qv + qhat.qv;
		new_qhat.qw = qw + qhat.qw;
		return new_qhat;
	}

	// Compound (In-Place) Addition
	// Adds one quaternion to the other, where this quaternion is
	// modified in-place
	Quaternion& Quaternion::operator+=(const Quaternion& qhat)
	{
		qv += qhat.qv;
		qw += qhat.qw;
		return *this;
	}

	// Multiplication
	// Takes the product of two quaternions, and returns 
	// the result of this product as a new quaternion
	Quaternion Quaternion::operator*(const Quaternion& qhat) const
	{
		Quaternion new_qhat;
		new_qhat.qv = Vector3::CrossProduct(qv, qhat.qv) + qhat.qv * qw + qv * qhat.qw;
		new_qhat.qw = qw * qhat.qw - Vector3::DotProduct(qv, qhat.qv);
		return new_qhat;
	}

	// Compound (In-Place) Multiplication
	// Multiplies a quaternion to this, modifying this quaternion
	// in place
	Quaternion& Quaternion::operator*=(const Quaternion& qhat)
	{
		qv = Vector3::CrossProduct(qv, qhat.qv) + qhat.qv * qw + qv * qhat.qw;
		qw = qw * qhat.qw - Vector3::DotProduct(qv, qhat.qv);
		return *this;
	}

	// Identity:
	// Returns the identity quaternion, with a 0 imaginary vector
	// and real component equal to 1
	Quaternion Quaternion::Identity()
	{
		return Quaternion(Vector3(0,0,0), 1);
	}

}
}