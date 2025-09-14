#pragma once

#include "../CreationCallback.h"
#include "../Object.h"

namespace Engine {
namespace Datamodel {
// Class DMLight:
// Represents a light in the datamodel.
class DMLight : public Object, public CreationCallback<DMLight> {
  public:
    DMLight();
    ~DMLight();
};

} // namespace Datamodel
} // namespace Engine