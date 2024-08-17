#include "AssetComponent.h"

#include "rendering/VisualSystem.h"

#include "rendering/Shader.h"

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
        curMesh = 0;
	}

    AssetComponent::~AssetComponent()
    {
        system->removeAssetComponent(this);
    }

    // BeginLoading:
    // Signals to the component that the visual system wants to begin using this asset
    // for a render pass
    void AssetComponent::beginLoading()
    {
        curMesh = 0;
    }

    // LoadMeshData:
    // Loads the data of a single mesh of the asset, and returns an int indicating if
    // data was successfully loaded. Returns the number of
    // indices to make a draw call for.
    // On any failure, like if there are no more meshes to render, returns -1.
    int AssetComponent::loadMeshData(ID3D11DeviceContext* context, CBHandle* cbHandle, ID3D11Device* device)
    {
        if (curMesh >= asset->getMeshes().size())
            return -1;
        else
        {
            // Bind CB data, which is per-mesh transformation matrices.
            Matrix4 worldTransform = object->getLocalMatrix();
            cbHandle->loadData(&worldTransform, FLOAT4X4);

            Matrix4 normalTransform = object->getLocalMatrix().inverse().transpose();
            cbHandle->loadData(&worldTransform, FLOAT4X4);

            // Load the data for one mesh. 
            Mesh& mesh = asset->getMeshes()[curMesh];
            int numIndices = mesh.loadIndexVertexData(context, device);
            
            curMesh++;

            return numIndices;
         }
        
    }

}
}