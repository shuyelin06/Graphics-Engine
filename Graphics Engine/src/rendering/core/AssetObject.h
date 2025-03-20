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
    Matrix4 m_local_to_world;

    AssetObject(Object* object, Asset* mesh);

  public:
    ~AssetObject();

    const Asset* getAsset() const;
    const Matrix4& getLocalToWorldMatrix() const;

    Vector3 getPosition() const;
    Quaternion getRotation() const;

    // Pulls the object transform
    void pullDatamodelData();
};

} // namespace Graphics
} // namespace Engine