#pragma once

#include "Asset.h"
#include "rendering/VisualObject.h"

namespace Engine {
namespace Graphics {
// MeshObject Class:
// Denotes an asset in the engine, that can be rendered.
class AssetObject : public VisualObject {
    friend class VisualSystem;

  private:
    Asset* asset;

    AssetObject(Object* object, Asset* mesh);

  public:
    ~AssetObject();

    const Asset* getAsset() const;
};

} // namespace Graphics
} // namespace Engine