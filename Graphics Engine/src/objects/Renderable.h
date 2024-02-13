#pragma once

#include "rendering/VisualEngine.h"
#include "rendering/buffers/Vertexbuffer.h"

namespace Engine
{
using namespace Graphics;

namespace Datamodel
{

	// Renderable Class
	// Can be used to produce renderable meshes
	class Renderable
	{
	private:
		static VertexBuffer cube_buffer;

	public:
		static VertexBuffer getCubeMesh(VisualEngine* graphics_engine);
	};
}
}