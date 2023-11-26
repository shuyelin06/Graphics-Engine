#pragma once

namespace Engine
{
	
	// VertexBuffer Class
	// Stores data and metadata regarding vertices, that can
	// be drawn to a screen
	class VertexBuffer
	{
	public:
		float* vertices; // Array of floats

		int vertex_size; // Number of floats per vertex
		int num_vertices; // Number of total vertices
	};
}