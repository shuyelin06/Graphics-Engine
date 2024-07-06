#include "AssetComponent.h"

#include "rendering/VisualSystem.h"

namespace Engine
{
using namespace Datamodel;

namespace Graphics
{
	// Constructor:
	// Saves a reference to the visual system for later rendering
	// purposes.
	AssetComponent::AssetComponent(Object* object, VisualSystem* _system, Asset* _asset)
		: VisualComponent(object, _system)
	{
        asset = _asset;
	}

    AssetComponent::~AssetComponent()
    {
        system->removeAssetComponent(this);
    }

	// RenderMesh:
	// Renders a mesh using the visual system.
    // Expects that the render target and output is already set before the render
    // takes place.
	void AssetComponent::render(VisualSystem* visual_system)
	{
        // Get Direct3D interfaces
        ID3D11DeviceContext* device_context = visual_system->getDeviceContext();
    
        // Bind Constant Buffer 2: 
        // Per-Mesh transform matrices
        AssetData asset_data = { };
        
        // Model -> World Space Transform (Vertices)
        asset_data.world_transform = object->getLocalMatrix();
        // Model -> World Space Transform (Normals)
        asset_data.normal_transform = object->getLocalMatrix().inverse().tranpose();

        visual_system->BindVSData(CB_Type::PER_ASSET, &asset_data, sizeof(AssetData));

        // Iterate through and render each mesh
        std::vector<Mesh>& meshes = asset->getMeshes();

        for (Mesh& mesh : meshes)
        {
            // Bind the mesh index and vertex buffers
            MeshBuffers buffers = visual_system->getMeshBuffers(&mesh, true);
            UINT vertex_stride = sizeof(MeshVertex);
            UINT vertex_offset = 0;
            UINT num_indices = mesh.triangleCount() * 3;

            // Configure input assembler
            device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            device_context->IASetInputLayout(visual_system->getInputLayout(0));

            device_context->IASetVertexBuffers(0, 1, &buffers.vertex_buffer, &vertex_stride, &vertex_offset);
            device_context->IASetIndexBuffer(buffers.index_buffer, DXGI_FORMAT_R32_UINT, 0);

            // Make a draw call
            device_context->DrawIndexed(num_indices, 0, 0);
        }
	}

}
}