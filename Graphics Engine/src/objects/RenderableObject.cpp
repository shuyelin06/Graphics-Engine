#include "RenderableObject.h"

namespace Engine
{
namespace Datamodel
{
	// Available renderable meshes
	static std::unordered_map<std::string, Mesh*> meshes;

	// Constructor:
	// Initializes the RenderableObject object
	RenderableObject::RenderableObject(std::string mesh_name)
	{
		mesh = meshes[mesh_name];
	}

	RenderableObject::RenderableObject() 
	{
		mesh = nullptr;
	}

	// SetMesh:
	// Sets the mesh used by the RenderableObject
	void RenderableObject::setMesh(Mesh* _mesh)
	{
		mesh = _mesh;
	}

	// GetMesh:
	// Returns the mesh (unmodifiable) for rendering purposes
	const Mesh* RenderableObject::getMesh() 
	{
		return mesh;
	};

	// NewMesh:
	// Adds a new mesh to the unordered_map cache
	void RenderableObject::newMesh(std::string id, Mesh* mesh)
	{
		meshes[id] = mesh;
	}

}
}