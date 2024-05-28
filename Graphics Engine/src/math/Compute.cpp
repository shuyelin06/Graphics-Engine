#include "Compute.h"

#include <math.h>

#include <stdlib.h>
#include <time.h>

// TODO: https://www.arendpeter.com/Perlin_Noise.html
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
	
	
	// Random:
	// Generates a random value within the range [low, high]
	float Compute::random(float low, float high) 
	{
		// Generate random float
		float rand_num = (float) (rand()) / RAND_MAX;
		return rand_num * (high - low) + low;
	}


}
}