#include "Mesh.h"

#include <assert.h>

#include "math/Vector3.h"

namespace Engine
{
using namespace Math;

namespace Graphics
{
	// Define Mesh Cache
	map<string, Mesh> Mesh::meshes = map<string, Mesh>();

	// LoadMeshes:
	// Load all meshes into the mesh cache
	void Mesh::LoadMeshes()
	{
		LoadCubeMesh();

		Mesh* mesh;

		parsePLYFile("data/Beethoven.ply", "Beethoven");
		mesh = GetMesh("Beethoven");
		mesh->setShaders(0, 0);
		mesh->calculateNormals();

		parsePLYFile("data/ketchup.ply", "Ketchup");
		mesh = GetMesh("Ketchup");
		mesh->setShaders(0, 0);
		mesh->calculateNormals();

		parsePLYFile("data/cube.ply", "Cube2");
		mesh = GetMesh("Cube2");
		mesh->setShaders(0, 0);
		mesh->calculateNormals();
	}

	// GetMesh:
	// Returns a mesh from the mesh cache
	Mesh* Mesh::GetMesh(const string name)
	{
		// Assert that mesh exists
		if (!meshes.contains(name))
			assert(false);
		// If it does, return the mesh
		else
			return &(meshes.at(name));
	}

	// VertexLayoutSize
	// Static method returning the number of floats a given 
	// vertex layout has
	int Mesh::VertexLayoutSize(char layout)
	{
		int size = ((layout & XYZ) * 3)	// 1st Bit: XYZ Position
			+ (((layout & RGB) >> 1) * 3)	// 2nd Bit: RGB Color
			+ (((layout & NORMAL) >> 2) * 3); // 3rd Bit: XYZ Normal
		return size;
	}

	// GenerateVertexLayout
	// Static method that lets us create a vertex layout, given
	// the input format 
	char Mesh::GenerateVertexLayout(bool pos, bool rgb, bool norm)
	{
		char layout = (pos & 1)				// 1st Bit: XYZ Position
					| ((rgb & 1) << 1)		// 2nd Bit: RGB Color
					| ((norm & 1) << 2);	// 3rd Bit: XYZ Normal
		return layout;
	}

	// Mesh Constructor:
	// Creates an empty mesh with a specified data layout
	Mesh::Mesh(char layout) 
	{
		// Save vertex layout
		vertex_layout = layout;

		// Reserve space for 3 vertices
		vertices.reserve(VertexLayoutSize(layout) * 3);
		indices.reserve(3);
	}

	// Mesh Constructor:
	// Creates an empty mesh
	Mesh::Mesh()
	{

	}

	// Mesh Accessors:
	// Access the fields of the Mesh class, but does not allow
	// for modification of the mesh
	const vector<float>& Mesh::getVertexBuffer() const
	{
		return vertices;
	}

	const vector<int>& Mesh::getIndexBuffer() const
	{
		return indices;
	}

	char Mesh::getVertexLayout() const
	{
		return vertex_layout;
	}

	char Mesh::getVertexShader() const
	{
		return vertex_shader;
	}

	char Mesh::getPixelShader() const
	{
		return pixel_shader;
	}

	// CalculateNormals:
	// Given the XYZ positions and indices, calculates the
	// normal vectors for each vertex
	void Mesh::calculateNormals() 
	{
		// Check that layout has XYZ position, and does NOT have
		// normals
		if ((vertex_layout & XYZ) != XYZ || (vertex_layout & NORMAL) == NORMAL)
			assert(false);

		// Create vector of vertex normals, where each 3 floats
		// correspond to the vertex at the same index
		int vertex_size = VertexLayoutSize(vertex_layout);
		int num_vertices = vertices.size() / vertex_size;
		
		vector<Vector3> vertex_normals;
		vertex_normals.resize(num_vertices); // Resize so that all floats are initially 0

		// Iterate through all faces and calculate the vertex normal
		for (int i = 0; i < indices.size(); i += 3)
		{
			// Calculate vertex normal
			int index_1 = indices[i];
			int index_2 = indices[i + 1];
			int index_3 = indices[i + 2];

			Vector3 v1 = Vector3(vertices[index_1 * vertex_size], vertices[index_1 * vertex_size + 1], vertices[index_1 * vertex_size + 2]);
			Vector3 v2 = Vector3(vertices[index_2 * vertex_size], vertices[index_2 * vertex_size + 1], vertices[index_2 * vertex_size + 2]);
			Vector3 v3 = Vector3(vertices[index_3 * vertex_size], vertices[index_3 * vertex_size + 1], vertices[index_3 * vertex_size + 2]);

			Vector3 normal = Vector3::CrossProduct(v2 - v1, v3 - v1);

			// Add this normal to all vertices involved
			vertex_normals[index_1] += normal;
			vertex_normals[index_2] += normal;
			vertex_normals[index_3] += normal;
		}

		// Normalize all normal vectors
		for (int i = 0; i < vertex_normals.size(); i++)
		{
			vertex_normals[i].inplaceNormalize();
		}
		
		// Recreate vertices vector
		vertex_layout |= NORMAL;
		int newSize = vertex_size + 3;
		
		vector<float> newVertices;
		newVertices.reserve(newSize * num_vertices);

		for (int i = 0; i < num_vertices; i++)
		{
			int offset = 0;

			// Add normals to each vertex, while accounting for order
			for (char layout_pin = 1; layout_pin != 0; layout_pin <<= 1)
			{
				// If layout contains the given pin, add to new vertices array
				if (layout_pin == NORMAL)
				{
					newVertices.push_back(vertex_normals[i].x);
					newVertices.push_back(vertex_normals[i].y);
					newVertices.push_back(vertex_normals[i].z);
				}
				// Otherwise, add the rest of the data
				else if ((vertex_layout & layout_pin) == layout_pin)
				{
					int num_floats = VertexLayoutSize(layout_pin);

					for (int j = 0; j < num_floats; j++)
					{
						newVertices.push_back(vertices[i * vertex_size + offset]);
						offset++;
					}
				}
			}

		}

		vertices = newVertices;
	}
	

	// SetShaders:
	// Sets the shaders to be used to render this mesh,
	// by their index in the VisualEngine
	void Mesh::setShaders(char vertex, char pixel)
	{
		vertex_shader = vertex;
		pixel_shader = pixel;
	}

	// AddVertex:
	// Adds a vertex to the Mesh's list of vertices.
	// Assumes the array of floats is the same size as the
	// vertex_layout provided
	void Mesh::addVertex(float vertex[]) 
	{
		int size = VertexLayoutSize(vertex_layout);

		for (int i = 0; i < size; i++) {
			vertices.push_back(vertex[i]);
		}
	}

	// AddIndex:
	// Adds an index to the Mesh's list of indices
	// (specifying vertex groups to be rendered).
	void Mesh::addIndex(int index)
	{
		indices.push_back(index);
	}

}
}