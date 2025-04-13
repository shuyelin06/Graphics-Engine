#pragma once

#include "Asset.h"
#include "datamodel/Component.h"

namespace Engine {
using namespace Datamodel;
namespace Graphics {
// MeshObject Class:
// Denotes an asset in the engine, that can be rendered.
class AssetComponent : public Component {
  private:
    Asset* asset;
    Matrix4 m_local_to_world;

  public:
    AssetComponent(Object* object, Asset* mesh);
    ~AssetComponent();

    const Asset* getAsset() const;
    const Matrix4& getLocalToWorldMatrix() const;

    Vector3 getPosition() const;
    Quaternion getRotation() const;

    // Pulls the object transform
    void pullDatamodelData();
};

} // namespace Graphics
} // namespace Engine