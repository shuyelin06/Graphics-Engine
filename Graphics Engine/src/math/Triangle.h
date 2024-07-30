#pragma once

#include "Vector3.h"

namespace Engine
{
namespace Math
{
	class Triangle
	{
	private:
		Vector3 vertices[3];

	public:
		Triangle();
		Triangle(const Vector3& v0, const Vector3& v1, const Vector3& v2);
		~Triangle();

		const Vector3& vertex(char vertex) const;

		Vector3 normal() const;
	};
}
}