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