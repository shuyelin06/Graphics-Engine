#include "rendering/Mesh.h"

namespace Engine
{
namespace Graphics
{
	void Mesh::LoadCubeMesh() 
	{
		Mesh mesh = Mesh(XYZ);

		// Generate vertices
		vector<float> vertices =
		{
			-0.5, -0.5, -0.5,  // Vertex 0
			0.5, -0.5, -0.5,   // Vertex 1
			0.5, 0.5, -0.5,    // Vertex 2
			-0.5, 0.5, -0.5,   // Vertex 3
			-0.5, -0.5, 0.5,   // Vertex 4
			0.5, -0.5, 0.5,    // Vertex 5
			0.5, 0.5, 0.5,     // Vertex 6
			-0.5, 0.5, 0.5     // Vertex 7
		};
		
		mesh.vertices = vertices;

		// Generate indices
		vector<int> indices =
		{
			0, 3, 2,    2, 1, 0,    // Front face
			1, 2, 6,    6, 5, 1,    // Right face
			5, 6, 7,    7, 4, 5,    // Back face
			4, 7, 3,    3, 0, 4,    // Left face
			3, 7, 6,    6, 2, 3,    // Top face
			4, 0, 1,    1, 5, 4    // Bottom face
		};

		mesh.indices = indices;

		mesh.vertex_shader = 0;
		mesh.pixel_shader = 0;

		mesh.calculateNormals();

		meshes["Cube"] = mesh;
	}
}
}