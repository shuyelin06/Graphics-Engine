#include "AssetComponent.h"

#include "datamodel/Object.h"

namespace Engine {
namespace Graphics {
AssetComponent::AssetComponent(Object* _object, Asset* _asset) {
    asset = _asset;
    object = _object;
}

AssetComponent::~AssetComponent() = default;

const Asset* AssetComponent::getAsset() const { return asset; }
const Matrix4& AssetComponent::getLocalToWorldMatrix() const {
    return m_local_to_world;
}

Object* AssetComponent::getObject() const { return object; }

// PullDatamodelData:
// Pulls the object transform from the datamodel.
void AssetComponent::update() { m_local_to_world = object->getLocalMatrix(); }

} // namespace Graphics
} // namespace Engine