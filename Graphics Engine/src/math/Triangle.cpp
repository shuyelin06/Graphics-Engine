#include "Triangle.h"

#include <assert.h>

namespace Engine
{
namespace Math
{
	// Constructors and Destructors:
	Triangle::Triangle()
		: vertices{ Vector3(), Vector3(), Vector3() }
	{}
	Triangle::Triangle(const Vector3& v0, const Vector3& v1, const Vector3& v2)
		: vertices{ v0, v1, v2 }
	{}
	Triangle::~Triangle() = default;
	
	// Vertex:
	// Returns one of the triangle's vertices. Assumes (without checks)
	// that the parameter is either 0, 1, or 2.
	const Vector3& Triangle::vertex(char vertex) const
	{
		assert(0 <= vertex && vertex <= 2);
		return vertices[vertex];
	}

	Vector3& Triangle::vertex(char vertex)
	{
		assert(0 <= vertex && vertex <= 2);
		return vertices[vertex];
	}

	// Center:
	// Returns the triangle's barycenter, obtained by
	// averaging all 3 vertices
	Vector3 Triangle::center() const
	{
		return (vertices[0] + vertices[1] + vertices[2]) / 3;
	}

	// Normal:
	// Returns the triangle's unit normal.
	Vector3 Triangle::normal() const
	{
		const Vector3 edge1 = vertices[1] - vertices[0];
		const Vector3 edge2 = vertices[2] - vertices[0];
		const Vector3 normal = edge1.cross(edge2);

		return normal.unit();
	}

}
}