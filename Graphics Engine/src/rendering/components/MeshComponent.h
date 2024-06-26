#pragma once

#include "datamodel/ComponentHandler.h"
#include "datamodel/Component.h"

#include "rendering/Mesh.h"

namespace Engine
{
namespace Graphics
{
	// Forward Declare of VisualSystem
	class VisualSystem;

	// MeshComponent Class:
	// Allows for the rendering of a triangular mesh in the scene.
	// The mesh component contains a variety of attributes that
	// affect what mesh is rendered, and how it is rendered.
	class MeshComponent
		: public Datamodel::Component<MeshComponent>
	{
	private:
		VisualSystem* visual_system;

		Mesh* mesh;

	public:
		MeshComponent(Datamodel::ComponentHandler<MeshComponent>* handler);

		// Sets the mesh to render
		void setMesh(Mesh* _mesh);

		// Renders a mesh using the VisualSystem.
		void renderMesh(const Matrix4& view_matrix, const Matrix4& projection_matrix);
	};
}
}