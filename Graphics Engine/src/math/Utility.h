#pragma once

namespace Engine
{

namespace Math
{

	#define PI 3.141592653589

	// Class Utility
	// Provides some utility functions for use
	// throughout the program
	class Utility
	{
	public:
		static float clamp(float value, float low, float high);
	};
}
}