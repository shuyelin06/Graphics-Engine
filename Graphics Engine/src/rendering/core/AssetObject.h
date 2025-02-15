#pragma once

#include "rendering/VisualObject.h"
#include "Asset.h"

namespace Engine {
namespace Graphics {
// AssetObject Class:
// Denotes an asset in the engine, that can be rendered.
class AssetObject : public VisualObject {
    friend class VisualSystem;

  private:
    Asset* asset;

    AssetObject(Object* object, Asset* asset);

  public:
    ~AssetObject();
    
    const Asset* getAsset();
};

} // namespace Graphics
} // namespace Engine