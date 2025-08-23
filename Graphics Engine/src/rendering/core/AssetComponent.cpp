#include "AssetComponent.h"

#include "datamodel/Object.h"

namespace Engine {
namespace Graphics {
AssetComponent::AssetComponent(Object* object, Asset* _asset)
    : Component(object) {
    asset = _asset;
}

AssetComponent::~AssetComponent() = default;

const Asset* AssetComponent::getAsset() const { return asset; }
const Matrix4& AssetComponent::getLocalToWorldMatrix() const {
    return m_local_to_world;
}

// PullDatamodelData:
// Pulls the object transform from the datamodel.
void AssetComponent::update() { m_local_to_world = object->getLocalMatrix(); }

} // namespace Graphics
} // namespace Engine