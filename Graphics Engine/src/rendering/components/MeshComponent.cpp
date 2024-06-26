#include "MeshComponent.h"

#include "rendering/VisualSystem.h"

namespace Engine
{
namespace Graphics
{
	// Constructor:
	// Saves a reference to the visual system for later rendering
	// purposes.
	MeshComponent::MeshComponent(Datamodel::ComponentHandler<MeshComponent>* handler)
		: Datamodel::Component<MeshComponent>(handler)
	{
        // TODO: HACK
		visual_system = static_cast<VisualSystem*>(handler);
        mesh = nullptr;
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
	void MeshComponent::renderMesh(const Matrix4& view_matrix, const Matrix4& projection_matrix)
	{
		// If mesh does not have any assignment, do nothing 
		if (mesh == nullptr)
			return;
		
        // If mesh has nothing, do nothing
        if (mesh->getIndexBuffer().size() == 0 || mesh->getVertexBuffer().size() == 0)
            return;

        // Get Direct3D interfaces
        ID3D11DeviceContext* device_context = visual_system->getDeviceContext();
    
        // Bind Vertex & Index Buffers:
        // Get the mesh's vertex and index vectors
        MeshBuffers buffers = visual_system->getMeshBuffers(mesh, true);
        UINT vertex_stride = Mesh::VertexLayoutSize(mesh->getVertexLayout()) * sizeof(float);
        UINT vertex_offset = 0;
        UINT num_indices = mesh->getIndexBuffer().size();

        device_context->IASetVertexBuffers(0, 1, &buffers.vertex_buffer, &vertex_stride, &vertex_offset);
        device_context->IASetIndexBuffer(buffers.index_buffer, DXGI_FORMAT_R32_UINT, 0);

        // Bind Constant Buffer 1: 
        // Transform matrices
        TransformData transform_data;

        Matrix4 m_modelToWorld = object->getLocalMatrix();
        Matrix4 m_worldToView = view_matrix;
        Matrix4 m_camera = projection_matrix;

        // Model -> World Space Transform (Vertices)
        transform_data.m_modelToWorld = m_modelToWorld;
        // Model -> Camera Space Transform
        transform_data.m_worldToCamera = m_worldToView * m_camera;
        // Model -> World Space Transform (Normals)
        transform_data.m_normalTransform = m_modelToWorld.inverse().tranpose();

        visual_system->BindVSData(0, &transform_data, sizeof(TransformData));

        /* Configure Input Assembler */
        // Define input layout
        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        device_context->IASetInputLayout(visual_system->getInputLayout(mesh->getVertexLayout()));

        /* Configure Shaders */
        // Bind vertex shader
        device_context->VSSetShader(visual_system->getVertexShader("Default"), NULL, 0);

        // Bind pixel shader
        device_context->PSSetShader(visual_system->getPixelShader("Default"), NULL, 0);

        // Make a draw call
        device_context->DrawIndexed(num_indices, 0, 0);
	}

}
}