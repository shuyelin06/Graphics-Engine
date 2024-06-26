#pragma once

#include <vector>

#include "math/Vector3.h"

namespace Engine
{
using namespace Math;

namespace Graphics
{
	// PointData Struct:
	// Contains data for a single point to be rendered (for debugging)
	// This data is loaded into a constant buffer for use with instancing
	struct PointData
	{
		Vector3 position;
		float scale;

		Vector3 color;
		float padding;
	};

	// LineData Struct:
	// Contains data for a single line to be rendered (for debugging)
	// This data is loaded into the vertex buffer for direct rendering
	struct LineData
	{
		Vector3 position_1;
		Vector3 position_2;
		Vector3 color;
	};

	// VisualDebug Class:
	// Contains methods that can be called statically
	// for convenient debugging purposes
	// Note: All debug rendering data is cleared after every frame
	class VisualDebug
	{
	friend class VisualSystem;

	private:
		static std::vector<PointData> points;
		static std::vector<LineData> lines;

	public:
		// Clear all debug rendering data
		static void Clear();

		// Draw a point in 3D space.
		static bool DrawPoint(const Vector3& position, float scale, const Vector3& color);

		// Draw a line in 3D space.
		static bool DrawLine(const Vector3& pos_1, const Vector3& pos_2, const Vector3& color);
	};
}
}