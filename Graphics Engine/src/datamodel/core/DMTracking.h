#pragma once

#include "DMEvent.h"
#include "DMListener.h"

namespace Engine {
namespace Datamodel {
void FireDatamodelEvent(const DMEvent& event);

class DMTrackedObject {
  private:
    const DMObjectHandle handle;
    const DMObjectTag object_tag;

  public:
    DMTrackedObject(const DMObjectTag& tag);
    ~DMTrackedObject();

    DMObjectHandle getHandle() const;
    DMObjectTag getObjectTag() const;
};

template <typename T> struct DMTrackedProperty {
  private:
    const DMTrackedObject* owner;
    const DMPropertyTag property_tag;

    T data;

  public:
    DMTrackedProperty(const DMTrackedObject* _owner, const DMPropertyTag& _tag)
        : owner(_owner), property_tag(_tag), data() {}

    void writeProperty(const T& new_value) {
        if (data != new_value) {
            data = new_value;

            DMEvent event;
            event.event_type = DMEventType::kPropertyUpdated;
            event.object = owner->getHandle();
            event.object_type = owner->getObjectTag();
            event.property_tag = property_tag;
            event.property_data = data;
            FireDatamodelEvent(event);
        }
    };

    T& readProperty() { return data; }
    const T& readProperty() const { return data; }
};

} // namespace Datamodel
} // namespace Engine