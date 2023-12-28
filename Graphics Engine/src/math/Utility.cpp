#include "Utility.h"

namespace Engine
{

namespace Math
{
	
	float Utility::clamp(float val, float low, float high)
	{
		return val < low ? low : ((val > high) ? high : val);
	}


}
}