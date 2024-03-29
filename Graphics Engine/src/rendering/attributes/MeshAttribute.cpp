#include "MeshAttribute.h"

namespace Engine
{
namespace Graphics
{
    std::map<std::string, Mesh> MeshAttribute::meshes = std::map<std::string, Mesh>();
    std::map<Mesh*, MeshBuffers> MeshAttribute::mesh_cache = std::map<Mesh*, MeshBuffers>();

	// Constructor:
	// Initializes a mesh attribute with
	// a given mesh
	MeshAttribute::MeshAttribute(Object* _object, Mesh* _mesh)
		: VisualAttribute(_object)
	{
		transform_matrix = Matrix4();
		mesh = _mesh;

        index_buffer = nullptr;
        vertex_buffer = nullptr;
	}
	
	// Destructor:
	// Destroys a mesh attribute
	MeshAttribute::~MeshAttribute()
	{

	}
	
    // GetTransformMatrix:
    // Helper method that finds an object's local_to_world transform
    // matrix
    Matrix4 MeshAttribute::GetTransformMatrix(Object* obj)
    {
        Object* parent = obj->getParent(VisualAttribute::GetObjectAccessor());
        Transform* transform = obj->getTransform(VisualAttribute::GetObjectAccessor());

        Matrix4 transform_matrix = transform->transformMatrix();

        if (parent != nullptr)
        {
            Matrix4 parent_mat = GetTransformMatrix(parent);
            return transform_matrix * parent_mat;
        }
        else
            return transform_matrix;
    }

	// Prepare:
	// Prepares a Mesh Attribute for rendering
    void MeshAttribute::prepare()
	{
		// Get object transforms
		Transform* obj_transform = object->getTransform(VisualAttribute::GetObjectAccessor());
		Transform* camera_transform = camera->getTransform(VisualAttribute::GetObjectAccessor());

		// Compute and save transform matrix for mesh vertices
        Matrix4 local_to_world = GetTransformMatrix(object);
		Matrix4 world_to_camera = GetTransformMatrix(camera).inverse();
		Matrix4 camera_to_project = camera->localToProjectionMatrix();

        rotate_matrix = obj_transform->rotationMatrix();
		transform_matrix = local_to_world * world_to_camera * camera_to_project;

        // Prepare vertex and index buffers for rendering
        // Bytes between each vertex 
        UINT vertex_stride = Mesh::VertexLayoutSize(mesh->getLayout()) * sizeof(float);
        // Offset into the vertex buffer to start reading from 
        UINT vertex_offset = 0;
        // Number of vertices
        UINT vertex_count = mesh->getVertices()->size();
        // Number of indices
        UINT num_indices = mesh->getIndices()->size();

        {
            // Get the mesh's vertex and index vectors
            const vector<float>* vertices = mesh->getVertices();
            const vector<int>* indices = mesh->getIndices();

            // Check if the vertex / index buffers have already been created
            // before. If they have, just use the already created resources
            if (mesh_cache.contains(mesh))
            {
                // Obtain buffers from cache
                MeshBuffers buffers = mesh_cache[mesh];

                // Use these buffers
                vertex_buffer = buffers.vertex_buffer;
                index_buffer = buffers.index_buffer;
            }
            // If they haven't, create these resources and add them to the cache
            // so we don't need to recreate them
            else
            {
                // Create new buffer resources
                vertex_buffer = create_buffer(
                    D3D11_BIND_VERTEX_BUFFER,
                    (void*)vertices->data(),
                    sizeof(float) * vertices->size());
                index_buffer = create_buffer(
                    D3D11_BIND_INDEX_BUFFER,
                    (void*)indices->data(),
                    sizeof(int) * indices->size());

                // Add buffer resources to cache
                mesh_cache[mesh] = { vertex_buffer, index_buffer };
            }
        }
	}

	// Render:
	// Renders a Mesh Attribute
	void MeshAttribute::render()
	{
        // Bytes between each vertex 
        UINT vertex_stride = Mesh::VertexLayoutSize(mesh->getLayout()) * sizeof(float);
        // Offset into the vertex buffer to start reading from 
        UINT vertex_offset = 0;
        // Number of vertices
        UINT vertex_count = mesh->getVertices()->size();
        // Number of indices
        UINT num_indices = mesh->getIndices()->size();

        // Bind transform matrix to the vertex shader
        bind_vs_data(0, transform_matrix.getRawData(), sizeof(float) * 16);
        bind_vs_data(1, rotate_matrix.getRawData(), sizeof(float) * 16);

        // Get shaders to render mesh with
        std::pair<ID3D11VertexShader*, ID3D11InputLayout*> vertex_shader = vertex_shaders[mesh->getVertexShader()];
        ID3D11PixelShader* pixel_shader = pixel_shaders[mesh->getPixelShader()];

        // Perform a Draw Call
        // Set the valid drawing area (our window)
        RECT winRect;
        GetClientRect(window, &winRect); // Get the windows rectangle

        D3D11_VIEWPORT viewport = {
            0.0f, 0.0f,
            (FLOAT)(winRect.right - winRect.left),
            (FLOAT)(winRect.bottom - winRect.top),
            0.0f,
            1.0f
        };

        // Set min and max depth for viewpoint (for depth testing)
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;

        // Give rectangle to rasterizer state function
        device_context->RSSetViewports(1, &viewport);

        // Set output merger to use our render target and depth test
        device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil);

        /* Configure Input Assembler */
        // Define input layout
        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        device_context->IASetInputLayout(vertex_shader.second);

        // Bind vertex and index buffers
        device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &vertex_stride, &vertex_offset);
        device_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);

        /* Configure Shaders*/
        // Bind vertex shader
        device_context->VSSetShader(vertex_shader.first, NULL, 0);

        // Bind pixel shader
        device_context->PSSetShader(pixel_shader, NULL, 0);

        // Draw from our vertex buffer
        device_context->DrawIndexed(num_indices, 0, 0);
	}
    
	void MeshAttribute::finish()
	{

	}
}
}