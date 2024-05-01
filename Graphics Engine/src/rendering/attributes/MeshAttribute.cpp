#include "MeshAttribute.h"

namespace Engine
{
namespace Graphics
{
	// Constructor:
	// Initializes a mesh attribute with
	// a given mesh
	MeshAttribute::MeshAttribute(Object* _object, Mesh* _mesh)
		: VisualAttribute(_object)
	{
		mesh = _mesh;
	}
	
	// Destructor:
	// Destroys a mesh attribute
	MeshAttribute::~MeshAttribute()
	{

	}
	
	// Prepare:
	// Prepares a Mesh Attribute for rendering
	void MeshAttribute::prepare()
	{

	}
}
}