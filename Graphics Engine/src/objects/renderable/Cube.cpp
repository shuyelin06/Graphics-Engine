#include "Cube.h"

#define COLOR(x) (x / 255.f)
#define NUMBER_INDICES 36

namespace Engine
{
namespace Datamodel
{
	float Cube::vertices[8 * VERTEX_SIZE] =
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
	
	Cube::Cube(float _size)
	{
		size = _size;

		// Vertices used by the cube
		int indices[NUMBER_INDICES] =
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
		for (int i = 0; i < NUMBER_INDICES; i++)
		{
			for (int j = 0; j < VERTEX_SIZE; j++)
			{
				index_mesh[i * VERTEX_SIZE + j] = Cube::vertices[indices[i] * VERTEX_SIZE + j];
				
				// XYZ Position
				if (j < 3) index_mesh[i * VERTEX_SIZE + j] *= size;
			}
		}
	}

	float triangle[3 * VERTEX_SIZE] =
	{
		1.5f, 0.f, 0.f, COLOR(108.f), COLOR(159.f), COLOR(125.f),
		0.f, 1.5f, 0.f, COLOR(25.f), COLOR(174.f), COLOR(134.f),
		0.0f, 0.0f, 0.0f, COLOR(194.f), COLOR(139.f), COLOR(16.f),
	};

	Graphics::VertexBuffer Cube::getVertexBuffer(void)
	{
		Graphics::VertexBuffer buffer;
		
		// buffer.vertices = triangle;
		// buffer.num_vertices = 3;
		// buffer.vertex_size = VERTEX_SIZE;

		buffer.vertices = index_mesh;
		buffer.num_vertices = NUMBER_INDICES;
		buffer.vertex_size = VERTEX_SIZE;

		return buffer;
	}
}
}