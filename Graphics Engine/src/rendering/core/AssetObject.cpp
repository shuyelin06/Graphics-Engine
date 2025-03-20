#include "AssetObject.h"

namespace Engine {
namespace Graphics {
AssetObject::AssetObject(Object* object, Asset* _asset) : VisualObject(object) {
    asset = _asset;
}

AssetObject::~AssetObject() = default;

const Asset* AssetObject::getAsset() const { return asset; }
const Matrix4& AssetObject::getLocalToWorldMatrix() const {
    return m_local_to_world;
}

Vector3 AssetObject::getPosition() const {
    return object->getTransform().getPosition();
}
Quaternion AssetObject::getRotation() const {
    return object->getTransform().getRotation();
}

// PullDatamodelData:
// Pulls the object transform from the datamodel.
void AssetObject::pullDatamodelData() {
    m_local_to_world = object->getLocalMatrix();
}

} // namespace Graphics
} // namespace Engine