#pragma once

#include "../CreationCallback.h"
#include "../Object.h"

namespace Engine {
namespace Datamodel {
// Class DMPhysics:
// Represents a light in the datamodel.
class DMPhysics : public Object, public CreationCallback<DMPhysics> {
  public:
    DMPhysics();
    ~DMPhysics();
};

} // namespace Datamodel
} // namespace Engine