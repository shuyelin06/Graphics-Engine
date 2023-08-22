#pragma once

#include "Vector3.h"

namespace Engine
{
	// Creates an AABB bounding box
	class Extents
	{
	public:
		Vector3 min;
		Vector3 max;

		Extents(Vector3 min, Vector3 max);
		~Extents() {};

		bool intersects(const Extents&);
	};
}