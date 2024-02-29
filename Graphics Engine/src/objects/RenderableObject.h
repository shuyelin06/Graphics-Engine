#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "objects/other/Camera.h"
#include "rendering/buffers/VertexBuffer.h"

namespace Engine
{
using namespace Graphics;

namespace Datamodel
{
	typedef std::vector<VertexBuffer> Mesh;

	// Class RenderableObject:
	// Implements an object that can be rendered
	// using the graphics engine
	class RenderableObject : public Object
	{
	protected:
		Mesh* mesh; // Renderable mesh

	public:
		RenderableObject(std::string mesh_name);
		RenderableObject();

		const Mesh* getMesh();
		void setMesh(Mesh* _mesh);

		// Add new mesh to the available meshes
		static void newMesh(std::string id, Mesh* mesh);
	};

	// Class Mesh

}
}