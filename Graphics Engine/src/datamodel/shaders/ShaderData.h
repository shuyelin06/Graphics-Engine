#pragma once

#include "math/Matrix4.h"
#include "math/Vector3.h"

namespace Engine
{
using namespace Math;

namespace Graphics
{
	typedef struct
	{
		Matrix4 cameraToWorld;
	};

} // Namespace Graphics

	typedef struct
	{
		Vector3 color;
		float PADDING;
	} ShaderData;
} // Namespace Engine