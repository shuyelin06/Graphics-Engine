#include "Triangle.h"

#include <assert.h>

namespace Engine
{
namespace Math
{
	Triangle::Triangle() = default;
	Triangle::Triangle(const Vector3& v0, const Vector3& v1, const Vector3& v2)
	{
		vertices[0] = v0;
		vertices[1] = v1;
		vertices[2] = v2;
	}
	Triangle::~Triangle() = default;
	
	const Vector3& Triangle::vertex(char vertex) const
	{
		assert(0 <= vertex && vertex <= 2);
		return vertices[vertex];
	}

	Vector3 Triangle::normal() const
	{
		Vector3 edge1 = vertices[1] - vertices[0];
		Vector3 edge2 = vertices[2] - vertices[0];

		return Vector3::CrossProduct(edge1, edge2);
	}

}
}