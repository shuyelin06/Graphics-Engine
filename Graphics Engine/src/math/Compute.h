#pragma once

namespace Engine
{

namespace Math
{

	#define PI 3.141592653589

	// Class Compute
	// Provides some utility math functions for use
	// throughout the program
	class Compute
	{
	public:
		// Forces value to be within the range [low, high]
		static float clamp(float value, float low, float high);
	};
}
}