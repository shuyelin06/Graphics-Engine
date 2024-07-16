#pragma once

#include "Direct3D11.h"

#include <vector>

#include "rendering/Shader.h"
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

	// VisualDebug Class:
	// Contains methods that can be called statically
	// for convenient debugging purposes
	// Note: All debug rendering data is cleared after every frame
	class VisualDebug
	{
	friend class VisualSystem;

	private:
		static std::vector<PointData> points;

	public:
		// Clear all debug rendering data
		static void Clear();

		// Quick and dirty rendering in 3D space
		static bool DrawPoint(const Vector3& position, float scale, const Vector3& color);

		// Pipeline Management
		static int LoadPointData(CBHandle* cbHandle);
	};
}
}