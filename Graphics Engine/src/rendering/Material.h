#pragma once

#include <string>

#include "math/Vector3.h"

namespace Engine 
{
namespace Graphics
{
	// Struct Material:
	// Maintains renderable properties for a mesh
	struct Material
	{
		// Ambient Color
		Math::Vector3 ka;
		// Diffuse Color
		Math::Vector3 kd;
		// Specular Color
		Math::Vector3 ks;

		// Transparency
		float tr;

		// Shininess
		float ns;

		// Illumination Model
		char illum;

		// Associated Texture
		std::string texture;
	};
}
}