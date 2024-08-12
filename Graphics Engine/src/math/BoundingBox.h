#pragma once

#include "Vector3.h"

namespace Engine
{
namespace Math
{
	// BoundingBox:
	// Represents an AABB in 3D space, given by its lower left corner and upper right
	// corner.
	class BoundingBox
	{
	private:
		Vector3 min;
		Vector3 max;

	public:
		BoundingBox();
		BoundingBox(const Vector3& center);
		~BoundingBox();

		void expandToContain(const Vector3& point);
	};

}
}