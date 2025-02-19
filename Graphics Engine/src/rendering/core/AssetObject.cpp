#include "AssetObject.h"

namespace Engine {
namespace Graphics {
AssetObject::AssetObject(Object* object, Asset* _asset) : VisualObject(object) {
    asset = _asset;
}

AssetObject::~AssetObject() = default;

const Asset* AssetObject::getAsset() const { return asset; }

} // namespace Graphics
} // namespace Engine