#pragma once

#include <map>
#include <string>

#include "Direct3D11.h"

#include "rendering/components/AssetLoader.h"
#include "rendering/components/AssetBuilder.h"
#include "rendering/components/Asset.h"

namespace Engine
{
namespace Graphics
{
	// Assets
	enum AssetSlot
	{
		Cube = 0,
		Fox = 1,
		Terrain = 2, // Temp
		AssetCount
	};

    enum TextureSlot
    {
        Test = 0, 
        Test2 = 1,
        TextureCount
    };

    // AssetWrapper:
    // Stores assets and their associated loader.
    // The loader is associated with the asset, and will
    // generate the GPU resources needed for the rendering 
    // pipeline
    struct AssetWrapper
    {
        Asset* asset;
        AssetLoader* loader;

        AssetWrapper();
        AssetWrapper(Asset* asset, ID3D11Device* device);
    };

	// AssetManager Class:
	// Manages assets for the engine. Provides methods
	// to load assets, and prepare them for rendering. 
	class AssetManager
	{
	private:
        ID3D11Device* device;

        std::vector<AssetWrapper> assets;
        std::vector<Texture*> textures;

	public:
		AssetManager(ID3D11Device* device);
		~AssetManager();

		// Initialize assets
		void initialize();

		// Get an asset data by name
        Asset* getAsset(AssetSlot asset);
        // Get an asset loader by name 
        AssetLoader* getAssetLoader(AssetSlot asset);
        
        // Get a texture by name
        Texture* getTexture(TextureSlot texture);

	private:
		// Generate a cube
		Asset* LoadCube();

		// Load an asset from an OBJ file. Returns the index of the
		// asset in the manager on success.
		Asset* LoadAssetFromOBJ(std::string path, std::string objFile, std::string assetName);

        bool LoadTextureFromPNG(TextureBuilder& builder, std::string path, std::string pngFile);
	};

}
}