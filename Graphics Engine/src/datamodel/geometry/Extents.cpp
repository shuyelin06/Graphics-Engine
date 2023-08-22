#include "Extents.h"

namespace Engine
{
	Extents::Extents(Vector3 _min, Vector3 _max)
	{
		min = Vector3(_min);
		max = Vector3(_max);
	}

	bool Extents::intersects(const Extents& extent)
	{
		bool x_intersect = !(max.x < extent.min.x || extent.max.x < min.x);
		bool y_intersect = !(max.y < extent.min.y || extent.max.y < min.y);
		bool z_intersect = !(max.z < extent.min.z || extent.max.z < min.z);

		return x_intersect && y_intersect && z_intersect;
	}
}