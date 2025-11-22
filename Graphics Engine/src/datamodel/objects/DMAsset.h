#pragma once

#include <string>

#include "../Bindable.h"
#include "../Object.h"

namespace Engine {
namespace Datamodel {
// DMAsset Class:
// Represents an asset in the scene.
class DMAsset : public Object, public Bindable<DMAsset> {
  private:
    std::string asset_name;

  public:
    DMAsset(const std::string& name);
    ~DMAsset();

    const std::string& getAssetName() const;
};

} // namespace Datamodel
} // namespace Engine