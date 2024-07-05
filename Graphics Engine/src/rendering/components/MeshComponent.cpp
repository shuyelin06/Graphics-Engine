#include "MeshComponent.h"

#include "rendering/VisualSystem.h"

namespace Engine
{
using namespace Datamodel;

namespace Graphics
{
	// Constructor:
	// Saves a reference to the visual system for later rendering
	// purposes.
	MeshComponent::MeshComponent(Object* object, VisualSystem* _system)
		: VisualComponent(object, _system)
	{
        mesh = nullptr;
	}

    MeshComponent::~MeshComponent()
    {
        system->removeMeshComponent(this);
    }

    // SetMesh:
    // Set the mesh to render
    void MeshComponent::setMesh(Mesh* _mesh)
    {
        mesh = _mesh;
    }

	// RenderMesh:
	// Renders a mesh using the visual system.
    // Expects that the render target and output is already set before the render
    // takes place.
	void MeshComponent::renderMesh(VisualSystem* visual_system)
	{
		// If mesh does not have any assignment, or is empty, do nothing 
		if (mesh == nullptr ||
            mesh->getIndexBuffer().size() == 0 || mesh->getVertexBuffer().size() == 0)
			return;

        // Get Direct3D interfaces
        ID3D11DeviceContext* device_context = visual_system->getDeviceContext();
    
        // Bind Constant Buffer 2: 
        // Per-Mesh transform matrices
        MeshData mesh_data = { };
        
        // Model -> World Space Transform (Vertices)
        mesh_data.world_transform = object->getLocalMatrix();
        // Model -> World Space Transform (Normals)
        mesh_data.normal_transform = object->getLocalMatrix().inverse().tranpose();

        visual_system->BindVSData(CB_Type::PER_INSTANCE, &mesh_data, sizeof(MeshData));

        // Bind Vertex & Index Buffers:
        // Get the mesh's vertex and index vectors
        MeshBuffers buffers = visual_system->getMeshBuffers(mesh, true);
        UINT vertex_stride = sizeof(MeshVertex);
        UINT vertex_offset = 0;
        UINT num_indices = mesh->getIndexBuffer().size();

        /* Configure Input Assembler */
        // Define input layout
        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        device_context->IASetInputLayout(visual_system->getInputLayout(0));

        device_context->IASetVertexBuffers(0, 1, &buffers.vertex_buffer, &vertex_stride, &vertex_offset);
        device_context->IASetIndexBuffer(buffers.index_buffer, DXGI_FORMAT_R32_UINT, 0);

        // Make a draw call
        device_context->DrawIndexed(num_indices, 0, 0);
	}

}
}