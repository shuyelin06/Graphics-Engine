#pragma once

#include "math/Matrix4.h"

#include "rendering/VisualAttribute.h";
#include "rendering/Mesh.h"

namespace Engine
{
using namespace Math;
namespace Graphics
{
	// Class MeshAttribute:
	// Implements the VisualAttribute class for
	// rendering meshes
	class MeshAttribute : public VisualAttribute
	{
	protected:
		Mesh* mesh;

	public:
		MeshAttribute(Object* object, Mesh* mesh);
		~MeshAttribute();

		// Prepares an object for rendering
		virtual void prepare() = 0;

		// Renders an object 
		virtual void render() = 0;

		// Finish the rendering for an object
		virtual void finish() = 0;
	};
}
}