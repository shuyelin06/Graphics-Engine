#pragma once

#include <string>
#include <vector>

#include "math/Vector3.h"
#include "math/Vector2.h"

namespace Engine
{
namespace Graphics
{
	// MeshVertex Struct:
	// Represents a vertex in the mesh. Vertices have 3
	// attributes: position, texture coordinate, and normal.
	// Position and normal will always be given, and texture coordinate
	// is optional.
	struct MeshVertex
	{
		Math::Vector3 position;
		Math::Vector2 textureCoord;
		Math::Vector3 normal;

		MeshVertex();
		MeshVertex(const Math::Vector3& pos, const Math::Vector2& tex, const Math::Vector3& norm);
	};

	// MeshTriangle Struct
	// Represents a triangle face of the mesh.
	// Vertex0, Vertex1, and Vertex2 represent indices into the vertex 
	// buffer. We do this to allow reuse of the same vertices, to be more memory
	// efficient.
	struct MeshTriangle
	{
		int vertex0;
		int vertex1;
		int vertex2;

		MeshTriangle();
		MeshTriangle(int v0, int v1, int v2);
	};

	// Struct Material:
	// Specifies renderable properties for a mesh
	struct Material
	{
		Math::Vector3 ka; // Ambient Color
		Math::Vector3 kd; // Diffuse Color
		Math::Vector3 ks; // Specular Color

		std::string texture; // Texture

		Material();
	};

	// Mesh Class
	// Stores information regarding vertices
	// and their index groupings to form a mesh.
	class Mesh
	{
	private:
		// Mesh renderable properties
		std::vector<MeshVertex> vertexBuffer;
		std::vector<MeshTriangle> indexBuffer;

		Material* material;

		// Determines if the mesh can be edited or not
		bool lock;

	public:
		// Mesh Constructor
		Mesh();

		// Mesh Initializers 
		bool setMaterial(Material* material);
		bool addVertex(MeshVertex& vertex);
		bool addTriangle(MeshTriangle& triangle);

		// Finalize mesh and lock it from further editing
		void lockMesh();

		// Mesh Accessors
		const std::vector<MeshVertex>& getVertexBuffer() const;
		const std::vector<MeshTriangle>& getIndexBuffer() const;

		int vertexCount() const;
		int triangleCount() const;
	};

	// Asset Class
	// Represents a renderable entity. Assets are composed of multiple
	// meshes, each of which can has a material. Together, these meshes
	// compose one renderable entity.
	class Asset
	{
	private:
		std::vector<Mesh> meshes;
		std::vector<Material> materials;
	
	public:
		Asset();
		~Asset();

		// Resource creation
		Mesh* newMesh();
		Material* newMaterial();
		
		// Resource accessing
		std::vector<Mesh>& getMeshes();
		std::vector<Material>& getMaterials();

		Mesh* getMesh(int mesh_index);
		Material* getMaterial(int material_index);
	};

	

	
}
}