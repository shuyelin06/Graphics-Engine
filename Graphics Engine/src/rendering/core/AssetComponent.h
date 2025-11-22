#pragma once

#include "datamodel/Object.h"
#include "Mesh.h"

namespace Engine {
using namespace Datamodel;
namespace Graphics {
// AssetComponent Class:
// Denotes an asset in the engine, that can be rendered.
class AssetComponent {
  private:
    Object* object;
    Asset* asset;
    Matrix4 m_local_to_world;

  public:
    AssetComponent(Object* object, Asset* mesh);
    ~AssetComponent();

    const Asset* getAsset() const;
    const Matrix4& getLocalToWorldMatrix() const;

    Object* getObject() const;

    // OVERRIDE: Pulls the object transform
    void update();
};

} // namespace Graphics
} // namespace Engine