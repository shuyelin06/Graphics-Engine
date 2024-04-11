#pragma once

#include "math/Matrix4.h"

#include "rendering/VisualAttribute.h"
#include "rendering/Mesh.h"

using namespace std;

namespace Engine
{
using namespace Math;
using namespace Datamodel;
namespace Graphics
{
	// MeshBuffers Struct:
	// Stores pointers to D3D11 Index/Vertex Buffers, which are mapped to 
	// Mesh pointers. Used to cache Index/Vertex Buffers, to avoid
	// redundantly recreating resources
	struct MeshBuffers
	{
		ID3D11Buffer* vertex_buffer;
		ID3D11Buffer* index_buffer;
	};

	// Class MeshAttribute:
	// Implements the VisualAttribute class for
	// rendering meshes
	class MeshAttribute : public VisualAttribute
	{
	protected:
		// Mesh Cache
		static map<std::string, Mesh> meshes;

		// Mesh Index/Vertex Buffer Cache
		static map<Mesh*, MeshBuffers> mesh_cache;

	private:
		static Matrix4 GetTransformMatrix(Object* object);

	protected:
		Matrix4 transform_matrix;
		Matrix4 rotate_matrix;
		Mesh* mesh;

		ID3D11Buffer* vertex_buffer;
		ID3D11Buffer* index_buffer;

	public:
		MeshAttribute(Object* object, Mesh* mesh);
		~MeshAttribute();

		// Prepares an object for rendering
		void prepare(void) override;

		// Renders an object 
		void render(void) override;

		// Finish the rendering for an object
		void finish(void) override;
	};
}
}