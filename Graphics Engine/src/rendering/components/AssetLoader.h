#pragma once

#include "rendering/Direct3D11.h"

#include "Asset.h"

namespace Engine
{
namespace Graphics 
{
    class AssetLoader;

    // MeshLoader
    // Stores information that 
    class MeshLoader
    {
    private:
        friend class AssetLoader;

        ID3D11Buffer* indexBuffer;
        ID3D11Buffer* vertexBuffer;

        int numTriangles;

    public:
        MeshLoader();
        ~MeshLoader();

        ID3D11Buffer* getIndexBuffer() const;
        ID3D11Buffer* getVertexBuffer() const;
        int getTriangleCount() const;
    };
    
    // Class AssetLoader
    // Given an asset, generates resources for the asset 
    // so that it can be loaded into the rendering pipeline.
    class AssetLoader
    {
    private:
        ID3D11Device* device;

        Asset* asset;

        std::vector<MeshLoader> meshLoaders;
        int meshIndex;

    public:
        AssetLoader(Asset* _asset, ID3D11Device* device);
        ~AssetLoader();

        // Returns the next mesh to load and render
        bool hasNextMesh();
        const MeshLoader& nextMesh();
        void reset();
    };
}
}