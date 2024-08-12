#include "Plane.h"

#include <math.h>
#include <algorithm>

namespace Engine
{
namespace Math
{
	Plane::Plane(const Vector3& _normal)
	{
		normal = _normal.unit();
		distance = 0;
	}
	Plane::Plane(const Vector3& _normal, const Vector3& _center)
	{
		normal = _normal.unit();
		distance = normal.dot(_center);
	}
	Plane::Plane(const Vector3& _normal, float _distance)
	{
		normal = _normal.unit();
		distance = _distance;
	}
	Plane::~Plane() = default;

	float Plane::distanceTo(const Vector3& point)
	{
		return std::abs(distanceToSigned(point));
	}
	float Plane::distanceToSigned(const Vector3& point)
	{
		return normal.dot(point) - distance;
	}

}
}