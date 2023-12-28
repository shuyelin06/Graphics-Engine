#pragma once

#include "rendering/Direct3D11.h"

namespace Engine
{

namespace Graphics
{
	// VertexBuffer Class
	// Stores data and metadata regarding vertices, that can
	// be drawn to a screen
	class VertexBuffer
	{
	public:
		ID3D11Buffer* vertex_buffer;

		int vertex_size; // Number of floats per vertex
		int num_vertices; // Number of total vertices
	};

}
}