#pragma once

#include "rendering/Asset.h"

#include "VisualComponent.h"

namespace Engine
{
namespace Graphics
{
	// Forward Declare of VisualSystem
	class VisualSystem;

	struct MeshData
	{
		Matrix4 world_transform;
		Matrix4 normal_transform;
	};

	// MeshComponent Class:
	// Allows for the rendering of a triangular mesh in the scene.
	// The mesh component contains a variety of attributes that
	// affect what mesh is rendered, and how it is rendered.
	class MeshComponent : public VisualComponent
	{
	private:
		Mesh* mesh;

	public:
		MeshComponent(Datamodel::Object* object, VisualSystem* system);
		~MeshComponent();

		// Sets the mesh to render
		void setMesh(Mesh* _mesh);

		// Renders a mesh using the VisualSystem.
		void renderMesh(VisualSystem* system);
	};
}
}