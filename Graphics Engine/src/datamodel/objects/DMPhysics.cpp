#include "DMPhysics.h"

namespace Engine {
namespace Datamodel {
DMPhysics::DMPhysics()
    : Object("Physics"), CreationCallback<DMPhysics>(this) {};
DMPhysics::~DMPhysics() = default;

} // namespace Datamodel
} // namespace Engine