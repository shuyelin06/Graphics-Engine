#include "AssetLoader.h"

#include <assert.h>

namespace Engine
{
namespace Graphics
{
    MeshLoader::MeshLoader() 
    {
        indexBuffer = NULL;
        vertexBuffer = NULL;
        numTriangles = 0;
    }
    MeshLoader::~MeshLoader() = default;

    ID3D11Buffer* MeshLoader::getIndexBuffer() const
    {
        return indexBuffer;
    }

    ID3D11Buffer* MeshLoader::getVertexBuffer() const
    {
        return vertexBuffer;
    }

    int MeshLoader::getTriangleCount() const
    {
        return numTriangles;
    }

    // Constructor:
    // Creates an asset loader. Given an asset, this loader can be called to
    // generate the resources for a given mesh and return it to be loaded
    // into the rendering pipeline
    AssetLoader::AssetLoader(Asset* _asset, ID3D11Device* _device)
    {
        meshLoaders.resize(_asset->getMeshes().size());

        device = _device;

        asset = _asset;
        meshIndex = 0;
    }
    AssetLoader::~AssetLoader() = default;

    // HasNextMesh:
    // Returns true if the loader has another mesh to
    // generate resources for
    bool AssetLoader::hasNextMesh()
    {
        const std::vector<Mesh>& meshes = asset->getMeshes();
        return meshIndex < meshes.size();
    }

    // NextMesh:
    // Generates and returns resources for a mesh to be loaded 
    const MeshLoader& AssetLoader::nextMesh()
    {
        Mesh* mesh = asset->getMesh(meshIndex);
        MeshLoader& loader = meshLoaders[meshIndex]; 

        meshIndex++;

        //if (!mesh->isStatic() || loader.indexBuffer == NULL || loader.vertexBuffer == NULL)
        {
            // Free existing resources
            if (loader.indexBuffer != NULL) 
                loader.indexBuffer->Release();
            if (loader.vertexBuffer != NULL)
                loader.vertexBuffer->Release();

            D3D11_BUFFER_DESC buff_desc = {};
            D3D11_SUBRESOURCE_DATA sr_data = { 0 };

            // Create resource for vertex buffer
            buff_desc.ByteWidth = sizeof(MeshVertex) * mesh->getVertexBuffer().size();
            buff_desc.Usage = D3D11_USAGE_DEFAULT;
            buff_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            sr_data.pSysMem = (void*)mesh->getVertexBuffer().data();

            device->CreateBuffer(&buff_desc, &sr_data, &(loader.vertexBuffer));
            assert(loader.vertexBuffer != nullptr);

            // Create resource for index buffer
            buff_desc.ByteWidth = sizeof(MeshTriangle) * mesh->getIndexBuffer().size();
            buff_desc.Usage = D3D11_USAGE_DEFAULT;
            buff_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            sr_data.pSysMem = (void*)mesh->getIndexBuffer().data();

            device->CreateBuffer(&buff_desc, &sr_data, &(loader.indexBuffer));
            assert(loader.indexBuffer != nullptr);

            loader.numTriangles = mesh->getIndexBuffer().size();
        }
        
        return loader;
    }

    void AssetLoader::reset()
    {
        meshIndex = 0;
    }
}
}