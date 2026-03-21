#include "DMTracking.h"

namespace Engine {
namespace Datamodel {
static uint32_t handle_counter = 0;
static DMListener* dm_listener = nullptr;
void RegisterDatamodelListener(DMListener* listener) { dm_listener = listener; }

void FireDatamodelEvent(const DMEvent& event) {
    if (dm_listener)
        dm_listener->onDatamodelEvent(event);
}

DMTrackedObject::DMTrackedObject(const DMObjectTag& tag)
    : handle(handle_counter++), object_tag(tag) {
    DMEvent event;
    event.event_type = DMEventType::kCreated;
    event.object = handle;
    event.object_type = object_tag;
    FireDatamodelEvent(event);
}
DMTrackedObject::~DMTrackedObject() {
    DMEvent event;
    event.event_type = DMEventType::kDestroyed;
    event.object = handle;
    event.object_type = object_tag;
    FireDatamodelEvent(event);
}

DMObjectHandle DMTrackedObject::getHandle() const { return handle; }
DMObjectTag DMTrackedObject::getObjectTag() const { return object_tag; }

} // namespace Datamodel
} // namespace Engine