#include "Vector2.h"

namespace Engine
{
namespace Math
{
	// Construtors and Destructor:
	Vector2::Vector2()
	{
		u = 0;
		v = 0;
	}

	Vector2::Vector2(const Vector2& copy)
	{
		u = copy.u;
		v = copy.v;
	}

	Vector2::Vector2(float _u, float _v)
	{
		u = _u;
		v = _v;
	}

	Vector2::~Vector2() = default;
}
}