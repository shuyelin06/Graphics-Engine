#include "VisualDebug.h"

namespace Engine
{
namespace Graphics
{
	// Initializing static fields
	std::vector<PointData> VisualDebug::points = std::vector<PointData>();

	// Clear:
	// Clears all debug data
	void VisualDebug::Clear()
	{
		points.clear();
	}

	// DrawPoint:
	// Registers a point in 3D space to be drawn by the visual
	// engine. Points are cleared after every frame
	bool VisualDebug::DrawPoint(const Vector3& position, float scale, const Vector3& color)
	{
		const int POINT_CAP = (4096 * 4 * sizeof(float)) / sizeof(PointData);

		// Check if there is space in the constant buffer for the point. If not
		// fail
		if (points.size() >= POINT_CAP)
		{
			return false;
		}
		// Otherwise, register point in the array
		else {
			PointData data;
			data.position = position;
			data.scale = scale;
			data.color = color;

			points.push_back(data);
			
			return true;
		}
		
	}

	// LoadPointData:
	// Loads the point data into a given constant buffer
	int VisualDebug::LoadPointData(CBHandle* cbHandle)
	{
		for (const PointData& data : points)
		{
			cbHandle->loadData(&data.position, FLOAT3);
			cbHandle->loadData(&data.scale, FLOAT);
			cbHandle->loadData(&data.color, FLOAT3);
		}

		return points.size();
	}
}
}