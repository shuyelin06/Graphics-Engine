#include "Color.h"

#include <assert.h>

namespace Engine
{
namespace Math
{
	Color::Color() : r(0), g(0), b(0) {}
	Color::Color(float _r, float _g, float _b)
	{
		r = _r;
		g = _g;
		b = _b;
	}
	Color::~Color() = default;

	Color Color::toRange1() const
	{
		assert(0 <= r <= 255);
		assert(0 <= g <= 255);
		assert(0 <= b <= 255);
		
		return Color(r / 255, g / 255, b / 255);
	}

	Color Color::toRange255() const
	{
		assert(0 <= r <= 1);
		assert(0 <= g <= 1);
		assert(0 <= b <= 1);
		
		return Color(r * 255, g * 255, b * 255);
	}

	// Generates some commonly used colors, for ease of use.
	Color Color::White()
		{ return Color(1, 1, 1); }
	Color Color::Red()
		{ return Color(1, 0, 0); }
	Color Color::Green()
		{ return Color(0, 1, 0); }
	Color Color::Blue()
		{ return Color(0, 0, 1); }
}
}