#include "Vector2.h"

namespace Engine
{
namespace Math
{
	// Constructors and Destructor:
	Vector2::Vector2()
		: x(0)
		, y(0)
	{}

	Vector2::Vector2(const Vector2& copy)
		: x(copy.x)
		, y(copy.y)
	{}

	Vector2::Vector2(float _x, float _y)
		: x(_x)
		, y(_y)
	{}

	Vector2::~Vector2() = default;

	// Dot:
	// Calculates the dot product between two 2D vectors.
	float Vector2::dot(const Vector2& vector) const
	{
		return x * vector.x + y * vector.y;
	}
}
}