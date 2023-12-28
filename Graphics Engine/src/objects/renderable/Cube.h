#pragma once

#include "Renderable.h"

#define VERTEX_SIZE 6

namespace Engine
{
namespace Datamodel
{

	// Cube Class 
	// Renderable cube
	class Cube : public Renderable
	{
	protected:
		static float vertices[8 * VERTEX_SIZE];

	private:
		float size;
		float index_mesh[36 * VERTEX_SIZE];

	public:
		Cube(float size);

		Graphics::VertexBuffer getVertexBuffer(void);
	};
}
}