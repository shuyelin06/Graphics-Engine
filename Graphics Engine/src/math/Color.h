#pragma once

namespace Engine
{
namespace Math
{
	// Color Class:
	// Represents an RGB color. Supports the 
	// the 0/1 and 0/255 RGB formats. 
	// By default, uses the 0/1 format.
	class Color
	{
	public:
		float r, g, b;

		Color();
		Color(float r, float g, float b);
		
		~Color();

		// Conversions between the 0/1 and 0/255 formats.
		Color toRange1() const;
		Color toRange255() const;

		// Commonly used colors
		static Color White();
		static Color Red();
		static Color Green();
		static Color Blue();

	};
}
}