#pragma once

namespace Engine
{
namespace Math
{
	// Vector2 Class:
	// Contains methods and data for a 2-dimensional
	// vector2.
	class Vector2
	{
	public:
		float u, v;

		Vector2();
		Vector2(const Vector2& copy);
		Vector2(float _u, float _v);

		~Vector2();
	};
}
}