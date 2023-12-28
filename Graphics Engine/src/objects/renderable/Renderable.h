#pragma once

#include "objects/Object.h"

#include "rendering/buffers/Vertexbuffer.h"

namespace Engine
{
namespace Datamodel
{

	// Renderable Class
	// Inherits object, and represents an object that can be
	// rendered to the graphics engine.
	class Renderable : public Object
	{
	public:
		Graphics::VertexBuffer getVertexBuffer(void);
	};
}
}