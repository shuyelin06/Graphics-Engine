#pragma once

#include "VisualComponent.h"

#include "rendering/Shader.h"
#include "rendering/Asset.h"

namespace Engine
{
namespace Graphics
{
	// Forward Declare of VisualSystem
	class VisualSystem;

	struct AssetData
	{
		Matrix4 world_transform;
		Matrix4 normal_transform;
	};

	struct MaterialData
	{

	};

	// MeshComponent Class:
	// Allows for the rendering of a triangular mesh in the scene.
	// The mesh component contains a variety of attributes that
	// affect what mesh is rendered, and how it is rendered.
	class AssetComponent : public VisualComponent
	{
	private:
		Asset* asset;
		
		int curMesh;

	public:
		AssetComponent(Datamodel::Object* object, VisualSystem* system, Asset* asset);
		~AssetComponent();

		// Pipeline Management
		// Resets the asset component for another render pass.
		void beginLoading();
		// Loads the data of a single mesh of the asset, and returns the number of indices to render.
		int loadMeshData(ID3D11DeviceContext* context, CBHandle* cbHandle, ID3D11Device* device);
	};
}
}