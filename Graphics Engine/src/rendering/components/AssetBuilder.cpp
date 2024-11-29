#include "AssetBuilder.h"

#include <assert.h>

namespace Engine
{
namespace Graphics
{
    MeshVertex::MeshVertex() = default;
    MeshVertex::MeshVertex(const MeshVertex& vertex)
    {
        position = vertex.position;
        textureCoord = vertex.textureCoord;
        normal = vertex.normal;
    }
    MeshVertex::MeshVertex(const Vector3& pos, const Vector2& tex, const Vector3& norm)
    {
        position = pos;
        textureCoord = tex;
        normal = norm;
    }

    MeshTriangle::MeshTriangle() = default;
    MeshTriangle::MeshTriangle(UINT v0, UINT v1, UINT v2)
    {
        vertex0 = v0;
        vertex1 = v1;
        vertex2 = v2;
    }

    MeshBuilder::MeshBuilder(ID3D11Device* _device)
    {
        device = _device;
    }
    MeshBuilder::~MeshBuilder() = default;

    // Generate:
    // Generates the index and vertex buffer resources for the
    // mesh.
    Mesh* MeshBuilder::generate()
    {
        if (index_buffer.size() == 0 || vertex_buffer.size() == 0)
            return nullptr;
        
        // My mesh to be returned
        Mesh* mesh = new Mesh();

        // Create resource for vertex buffer
        D3D11_BUFFER_DESC buff_desc = {};
        D3D11_SUBRESOURCE_DATA sr_data = { 0 };

        buff_desc.ByteWidth = sizeof(MeshVertex) * vertex_buffer.size();
        buff_desc.Usage = D3D11_USAGE_DEFAULT;
        buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        sr_data.pSysMem = (void*) vertex_buffer.data();

        device->CreateBuffer(&buff_desc, &sr_data, &(mesh->vertex_buffer));
        assert(mesh->vertex_buffer != nullptr);

        // Create resource for index buffer
        buff_desc.ByteWidth = sizeof(MeshTriangle) * index_buffer.size();
        buff_desc.Usage = D3D11_USAGE_DEFAULT;
        buff_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        sr_data.pSysMem = (void*) index_buffer.data();

        device->CreateBuffer(&buff_desc, &sr_data, &(mesh->index_buffer));
        assert(mesh->index_buffer != nullptr);

        mesh->triangle_count = index_buffer.size();

        return mesh;
    }

    // AddVertex:
    // Adds a vertex with position, texture, and norm to the MeshBuilder.
    UINT MeshBuilder::addVertex(const Vector3& pos, const Vector2& tex, const Vector3& norm)
    {
        UINT index = vertex_buffer.size();
        vertex_buffer.push_back(MeshVertex(pos, tex, norm));
        return index;
    }

    // AddTriangle:
    // Adds a triangle to the MeshBuilder with indices specified by the parameters.
    void MeshBuilder::addTriangle(UINT v1, UINT v2, UINT v3)
    {
        index_buffer.push_back(MeshTriangle(v1,v2,v3));
    }

    // RegenerateNormals:
    // Discard the current normals for the mesh and regenerate them
    void MeshBuilder::regenerateNormals()
    {
		// Regenerate mesh normals. We do this by calculating the normal 
		// for each triangle face, and adding them to a vector to 
		// accumulate their contribution to each vertex
		std::vector<Vector3> meshNormals;
		meshNormals.resize(vertex_buffer.size());

		for (int i = 0; i < index_buffer.size(); i++)
		{
			// Calculate vertex normal
			const MeshTriangle& triangle = index_buffer[i];

			const Vector3& vertex0 = vertex_buffer[triangle.vertex0].position;
			const Vector3& vertex1 = vertex_buffer[triangle.vertex1].position;
			const Vector3& vertex2 = vertex_buffer[triangle.vertex2].position;

			Vector3 normal = (vertex1 - vertex0).cross(vertex2 - vertex0);

			// Add this normal's contribution for all vertices of the face
			meshNormals[triangle.vertex0] += normal;
			meshNormals[triangle.vertex1] += normal;
			meshNormals[triangle.vertex2] += normal;
		}

		// Iterate through all vertices in the mesh. If their normal is degenerate (0,0,0),
		// replace it with the generated normal.
		for (int i = 0; i < vertex_buffer.size(); i++)
		{
			Vector3& normal = vertex_buffer[i].normal;

			if (normal.magnitude() == 0)
			{
				meshNormals[i].inplaceNormalize();
                vertex_buffer[i].normal = meshNormals[i];
			}
		}

    }
 
    // Reset:
    // Clears the MeshBuilder so it can be used to generate another mesh
    void MeshBuilder::reset()
    {
        vertex_buffer.clear();
        index_buffer.clear();
    }

    TextureBuilder::TextureBuilder(ID3D11Device* _device, unsigned int _width, unsigned int _height)
    {
        device = _device;

        pixel_width = _width;
        pixel_height = _height;

        data.resize(pixel_width * pixel_height);
        clear( {90,34,139,255} ); 
    }

    TextureBuilder::~TextureBuilder() = default;

    // GenerateTexture:
    // Generates a texture resource (for use in the rendering pipeline)
    // given the data stored within the builder.
    Texture* TextureBuilder::generate()
    {
        Texture* texture_resource = new Texture();
        texture_resource->width = pixel_width;
        texture_resource->height = pixel_height;

        // Generate my GPU texture resource
        D3D11_TEXTURE2D_DESC tex_desc = {};
        tex_desc.Width = pixel_width;
        tex_desc.Height = pixel_height;
        tex_desc.MipLevels = tex_desc.ArraySize = 1;
        tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        tex_desc.SampleDesc.Count = 1;
        tex_desc.Usage = D3D11_USAGE_DEFAULT;
        tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        tex_desc.CPUAccessFlags = 0;

        D3D11_SUBRESOURCE_DATA sr_data = {};
        sr_data.pSysMem = data.data();
        sr_data.SysMemPitch = pixel_width * 4;                      // Bytes per row
        sr_data.SysMemSlicePitch = pixel_width * pixel_height * 4;  // Total byte size

        device->CreateTexture2D(&tex_desc, &sr_data, &(texture_resource->texture));
        assert(texture_resource->texture != NULL);

        // Generate a shader view for my texture
        D3D11_SHADER_RESOURCE_VIEW_DESC tex_view;
        tex_view.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        tex_view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        tex_view.Texture2D.MostDetailedMip = 0;
        tex_view.Texture2D.MipLevels = 1;

        device->CreateShaderResourceView(texture_resource->texture, &tex_view, &(texture_resource->view));

        return texture_resource;
    }
    

    // SetColor:
    // Sets a pixel of the texture to some color value
    void TextureBuilder::setColor(unsigned int x, unsigned int y, const TextureColor& rgba)
    {
        assert(0 <= x && x < pixel_width);
        assert(0 <= y && y < pixel_height);

        data[y * pixel_width + x] = rgba;
    }

    // Clear:
    // Clears the texture, setting all of the RGBA pixels to a particular color.
    void TextureBuilder::clear(const TextureColor& rgba)
    {
        for (int i = 0; i < pixel_width * pixel_height; i++)
            data[i] = rgba;
    }

    // Reset:
    // Resets the builder
    void TextureBuilder::reset(unsigned int _width, unsigned int _height)
    {
        pixel_width = _width;
        pixel_height = _height;

        data.resize(pixel_width * pixel_height);
        clear({ 90,34,139,255 });
    }

}
}