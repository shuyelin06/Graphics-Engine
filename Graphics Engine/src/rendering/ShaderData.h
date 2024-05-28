#pragma once

#include "math/Matrix4.h"

// ShaderData.h
// Contains structs that store data to be passed into shader
// constant buffers.
// Note that data in the structs MUST be aligned by 4 bytes.
namespace Engine
{
using namespace Math;

namespace Graphics
{
	// TransformData
	// Contains data needed to transform vertices and normals
	// within the vertex shader
	struct TransformData
	{
		// 4 x 4 = 16 floats
		Matrix4 m_modelToWorld;	   // Model -> World Space
		
		// 4 x 4 = 16 floats
		Matrix4 m_worldToCamera;   // Model -> Camera Space for Vertices
		
		// 4 x 4 = 16 floats
		Matrix4 m_normalTransform; // Normal Transform
	};

	// LightData
	// Contains data for a single light source in the scene
	struct LightData
	{
		Vector3 position;
		float padding;
	};

	// PointData
	// Contains data for a single point to be rendered (for debugging)
	struct PointData
	{
		Vector3 position;
		float padding;
		Vector3 color;
		float padding2;
		float scale;
		Vector3 padding3;
	};
}
}