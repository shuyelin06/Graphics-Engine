#include "DMPhysics.h"

namespace Engine {
namespace Datamodel {
DMPhysics::DMPhysics() : Object(), Bindable<DMPhysics>(this) {
    DMPhysics::SignalObjectCreation(this);
};
DMPhysics::~DMPhysics() = default;

} // namespace Datamodel
} // namespace Engine