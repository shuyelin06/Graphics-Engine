#pragma once

#include <map>
#include <string>

#include "Asset.h"

namespace Engine
{
namespace Graphics
{
	// AssetManager Class:
	// Manages assets for the engine. Provides methods
	// to load assets, manipulate them, and more. 
	class AssetManager
	{
	private:
		std::map<std::string, Asset*> assets;

	public:
		AssetManager();
		~AssetManager();

		// Load an asset from an OBJ file. Returns the index of the
		// asset in the manager on success.
		void LoadAssetFromOBJ(std::string path, std::string objFile, std::string assetName);
	};

}
}