#include "Compute.h"

namespace Engine
{

namespace Math
{
	
	// Clamp:
	// Forces value to be within the range [low, high]
	float Compute::clamp(float val, float low, float high)
	{
		const float temp = val < low ? low : val;
		return temp > high ? high : temp;
	}


}
}