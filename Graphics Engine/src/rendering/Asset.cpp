#include "Asset.h"

namespace Engine
{
using namespace Math;

namespace Graphics
{
	// MeshVertex Constructors
	MeshVertex::MeshVertex() = default;

	MeshVertex::MeshVertex(const Vector3& pos, const Vector2& tex, const Vector3& norm)
	{
		position = pos;
		textureCoord = tex;
		normal = norm;
	}

	// MeshTriangle Constructors
	MeshTriangle::MeshTriangle()
	{
		vertex0 = -1;
		vertex1 = -1;
		vertex2 = -1;
	}

	MeshTriangle::MeshTriangle(int v0, int v1, int v2)
	{
		vertex0 = v0;
		vertex1 = v1;
		vertex2 = v2;
	}

	// Material Constructor
	Material::Material()
	{
		ka = Vector3(0.2f, 0.2f, 0.2f);
		kd = Vector3(0.8f, 0.8f, 0.8f);
		ks = Vector3(1.f, 1.f, 1.f);

		texture = std::string();
	}

	// Mesh Constructor:
	// Creates an empty mesh
	Mesh::Mesh()
	{
		vertexBuffer = std::vector<MeshVertex>();
		indexBuffer = std::vector<MeshTriangle>();

		material = nullptr;
		lock = false;
	}

	// Mesh Initializers
	// Allows editing of the mesh while it is not locked. Meshes start
	// unlocked, so that the asset manager can modify them.
	// After they are locked, they cannot be edited.
	bool Mesh::setMaterial(Material* _material)
	{
		if (lock)
			return false;
		material = _material;
	}

	bool Mesh::addVertex(MeshVertex& vertex)
	{
		if (lock)
			return false;
		else
		{
			vertexBuffer.push_back(vertex);
			return true;
		}
	}

	bool Mesh::addTriangle(MeshTriangle& triangle)
	{
		if (lock)
			return false;
		else
		{
			indexBuffer.push_back(triangle);
			return true;
		}
	}

	// LockMesh:
	// Finalize the mesh format and prevent further editing of the mesh.
	// This method will fix any degenerate normals by manually generating new
	// ones, and toggle the mesh lock.
	void Mesh::lockMesh()
	{
		// Toggle the mesh lock
		lock = true;

		// Regenerate mesh normals. We do this by calculating the normal 
		// for each triangle face, and adding them to a vector to 
		// accumulate their contribution to each vertex
		std::vector<Vector3> meshNormals;
		meshNormals.resize(vertexBuffer.size());

		for (int i = 0; i < indexBuffer.size(); i++)
		{
			// Calculate vertex normal
			const MeshTriangle& triangle = indexBuffer[i];

			const Vector3& vertex0 = vertexBuffer[triangle.vertex0].position;
			const Vector3& vertex1 = vertexBuffer[triangle.vertex1].position;
			const Vector3& vertex2 = vertexBuffer[triangle.vertex2].position;

			Vector3 normal = Vector3::CrossProduct(vertex1 - vertex0, vertex2 - vertex0);

			// Add this normal's contribution for all vertices of the face
			meshNormals[triangle.vertex0] += normal;
			meshNormals[triangle.vertex1] += normal;
			meshNormals[triangle.vertex2] += normal;
		}

		// Iterate through all vertices in the mesh. If their normal is degenerate (0,0,0),
		// replace it with the generated normal.
		for (int i = 0; i < vertexBuffer.size(); i++)
		{
			Vector3& normal = vertexBuffer[i].normal;

			if (normal.magnitude() == 0)
			{
				meshNormals[i].inplaceNormalize();
				vertexBuffer[i].normal = meshNormals[i];
			}
		}
	}

	
	// Mesh Accessors:
	// Access the fields of the Mesh class, but does not allow
	// for modification of the mesh
	const std::vector<MeshVertex>& Mesh::getVertexBuffer() const
	{
		return vertexBuffer;
	}
	
	const std::vector<MeshTriangle>& Mesh::getIndexBuffer() const
	{
		return indexBuffer;
	}

	int Mesh::vertexCount() const
	{
		return vertexBuffer.size();
	}

	// Asset Class
	// Represents a collection of meshes and materials. An object
	// can register an asset to obtain a renderable entity.
	Asset::Asset() = default;
	Asset::~Asset() = default;

	// Creates a new material or mesh, and returns the index to it. 
	// Used during asset creation.
	Mesh* Asset::newMesh()
	{
		meshes.push_back(Mesh());
		return &(meshes[meshes.size() - 1]);
	}

	Material* Asset::newMaterial()
	{
		materials.push_back(Material());
		return &(materials[materials.size() - 1]);
	}

	// Get a particular mesh or material in the asset.
	Mesh* Asset::getMesh(int mesh_index)
	{
		return &meshes[mesh_index];
	}

	Material* Asset::getMaterial(int material_index)
	{
		return &materials[material_index];
	}

}
}