#include "BoundingBox.h"

namespace Engine
{
namespace Math
{
	BoundingBox::BoundingBox()
		: min(Vector3::VectorMax())
		, max(Vector3::VectorMin())
	{
	}
	BoundingBox::BoundingBox(const Vector3& center)
		: min(center)
		, max(center)
	{
	}
	BoundingBox::~BoundingBox() = default;

	void BoundingBox::expandToContain(const Vector3& point)
	{
		min = min.componentMin(point);
		max = max.componentMax(point);
	}
}
}