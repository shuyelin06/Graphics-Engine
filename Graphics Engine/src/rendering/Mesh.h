#pragma once

#include <vector>
#include <map>

#include <string>

namespace Engine
{
namespace Graphics
{
	using namespace std;
	
	// Uses bitwise pins to represent the data layout
	// of each vertex. Assumes data is given from least
	// to most significant bit (Right -> Left)
	// >>> 1st Bit) Position (X,Y,Z)
	// >>> 2nd Bit) Color (R,G,B)
	// >>> 3rd Bit) Normal (X,Y,Z)
	// Input data MUST be in this order.
	enum VertexLayout { 
		XYZ = 1, 
		RGB = (1 << 1), 
		NORMAL = (1 << 2) 
	};
	
	// Mesh Class
	// Stores information regarding vertices
	// and their index groupings to form a mesh.
	class Mesh
	{
	private:
		// Renderable Mesh Cache
		static map<string, Mesh> meshes;

	private:
		// Vertex Data Layout 
		char vertex_layout;
		
		// Vertex buffer and index buffer
		vector<float> vertices;
		vector<int> indices;

		// Shader to render this mesh with
		char vertex_shader;
		char pixel_shader;
	
	public:
		// Load All Meshes
		static void LoadMeshes();

		// Get Meshes
		static Mesh* GetMesh(const string name);

		// Number of floats a vertex_layout has
		static int VertexLayoutSize(char layout);
		static char GenerateVertexLayout(bool pos, bool rgb, bool norm);

		// Mesh generation
		static void LoadCubeMesh();
		static void parsePLYFile(string ply_file, string mesh_name);
		
		// Mesh Constructor
		Mesh();
		Mesh(char layout);

		// Mesh Accessors
		const vector<float>& getVertexBuffer() const;
		const vector<int>& getIndexBuffer() const;

		char getVertexLayout() const;
		char getVertexShader() const;
		char getPixelShader() const;

		// Adds vertex normals to the mesh based on position,
		// if they don't already exist
		void calculateNormals();

		// Set shaders (by index) to render this mesh with
		void setShaders(char vertex, char pixel);

	private:
		// Add vertices or indices to the mesh
		void addVertex(float vertex[]);
		void addIndex(int index);
	};

}
}