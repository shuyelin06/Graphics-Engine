#include "DMAsset.h"

namespace Engine {
namespace Datamodel {
DMAsset::DMAsset(const std::string& name)
    : Object(), Bindable<DMAsset>(this), asset_name(name) {
    DMAsset::SignalObjectCreation(this);
}
DMAsset::~DMAsset() = default;

const std::string& DMAsset::getAssetName() const { return asset_name; }

} // namespace Datamodel
} // namespace Engine