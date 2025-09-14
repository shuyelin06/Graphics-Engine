#pragma once

#include "../Bindable.h"
#include "../Object.h"

namespace Engine {
namespace Datamodel {
// Class DMPhysics:
// Represents a light in the datamodel.
class DMPhysics : public Object, public Bindable<DMPhysics> {
  public:
    DMPhysics();
    ~DMPhysics();
};

} // namespace Datamodel
} // namespace Engine