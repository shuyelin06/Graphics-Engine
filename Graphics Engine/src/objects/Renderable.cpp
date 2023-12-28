#include "Renderable.h"

#define COLOR(x) (x / 255.f)

namespace Engine
{
namespace Datamodel
{
	// Generates a Cube Mesh
	VertexBuffer Renderable::cube_buffer = { 0 };
	
	VertexBuffer Renderable::getCubeMesh()
	{
		#define CUBE_VERTEX_SIZE 6
		#define CUBE_NUM_VERTICES 36

		// Singleton; return the already created cube buffer if it exists
		if (cube_buffer.vertex_buffer != NULL)
			return cube_buffer;

		// Vertices of the cube
		float vertices[8 * CUBE_VERTEX_SIZE] =
		{
			-1.0f, 1.0f, -1.0f, COLOR(108.f), COLOR(159.f), COLOR(125.f),
			 1.0f, 1.0f, -1.0f, COLOR(25.f), COLOR(174.f), COLOR(1134.f),
			 -1.0f, -1.0f, -1.0f, COLOR(194.f), COLOR(139.f), COLOR(16.f),
			1.0f, -1.0f, -1.0f, COLOR(255.f), COLOR(14.f), COLOR(198.f),
			-1.0f, 1.0f, 1.0f, COLOR(34.f), COLOR(255.f), COLOR(158.f),
			1.0f, 1.0f, 1.0f, COLOR(26.f), COLOR(101.f), COLOR(231.f),
			-1.0f, -1.0f, 1.0f, COLOR(6.f), COLOR(188.f), COLOR(130.f),
			 1.0f, -1.0f, 1.0f, COLOR(194.f), COLOR(200.f), COLOR(162.f)
		};

		// Vertices used by the cube
		int indices[CUBE_NUM_VERTICES] =
		{
			0, 1, 2,    // side 1
			2, 1, 3,
			4, 0, 6,    // side 2
			6, 0, 2,
			7, 5, 6,    // side 3
			6, 5, 4,
			3, 1, 7,    // side 4
			7, 1, 5,
			4, 5, 0,    // side 5
			0, 5, 1,
			3, 7, 2,    // side 6
			2, 7, 6
		};

		// Initialize list of vertices
		float index_mesh[CUBE_NUM_VERTICES * CUBE_VERTEX_SIZE];

		for (int i = 0; i < CUBE_NUM_VERTICES; i++)
			for (int j = 0; j < CUBE_VERTEX_SIZE; j++)
				index_mesh[i * CUBE_VERTEX_SIZE + j] = vertices[indices[i] * CUBE_VERTEX_SIZE + j];

		cube_buffer = graphics_engine.generate_vertex_buffer(index_mesh, CUBE_VERTEX_SIZE, CUBE_NUM_VERTICES);
		
		return cube_buffer;
	}
}
}