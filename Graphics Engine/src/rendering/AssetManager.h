#pragma once

#include <map>
#include <string>

#include "Asset.h"

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

	// AssetManager Class:
	// Manages assets for the engine. Provides methods
	// to load assets, manipulate them, and more. 
	class AssetManager
	{
	private:
		std::vector<Asset*> assets;

	public:
		AssetManager();
		~AssetManager();

		// Initialize assets
		void initialize();

		// Get an asset by name
		Asset* getAsset(AssetSlot asset);

	private:
		// Generate a cube
		Asset* LoadCube();


		// Load an asset from an OBJ file. Returns the index of the
		// asset in the manager on success.
		Asset* LoadAssetFromOBJ(std::string path, std::string objFile, std::string assetName);
	};

}
}