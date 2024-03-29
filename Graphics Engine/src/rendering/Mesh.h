#pragma once

#include <vector>
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
		// Accessible by the VisualEngine class for rendering
		friend class VisualAttribute;
		// friend class MeshAttribute;

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
		// Number of floats a vertex_layout has
		static int VertexLayoutSize(char layout);
		static char GenerateVertexLayout(bool pos, bool rgb, bool norm);

		// File parsers that can generate meshes
		static Mesh parsePLYFile(string ply_file);

		// Mesh Constructor
		Mesh(char layout);

		// Get (Unmodifiable) Information
		int getVertexShader();
		int getPixelShader();

		char getLayout();
		const vector<float>* getVertices();
		const vector<int>* getIndices();

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