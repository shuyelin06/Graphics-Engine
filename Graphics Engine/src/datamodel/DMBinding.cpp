#include "DMBinding.h"

#include "Object.h"

namespace Engine {
namespace Datamodel {
DMBinding::DMBinding(Object* obj) {
    dm_object = obj;
    dm_object->bind(this);
}
DMBinding::~DMBinding() {
    // On destruction, notify the bound object that the
    // binding is gone
    if (dm_object != nullptr) {
        dm_object->unbind();
        dm_object = nullptr;
    }
};

// Disconnect / ShouldDestroy:
// Unbinds this from the object. After this is called, it should be cleaned up.
void DMBinding::unbind() { dm_object = nullptr; }
bool DMBinding::shouldDestroy() const { return dm_object == nullptr; }

// PullDatamodelData:
// Main interfacing function to pull datamodel data. We use a function
// so that we can enforce when the datamodel is being accessed.
// To pull, override pullDatamodelDataImpl.
void DMBinding::pullDatamodelData() { pullDatamodelDataImpl(dm_object); }
void DMBinding::pullDatamodelDataImpl(Object* object) {}

} // namespace Datamodel
} // namespace Engine