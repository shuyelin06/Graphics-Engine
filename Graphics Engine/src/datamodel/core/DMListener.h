#pragma once

#include "DMEvent.h"

namespace Engine {
namespace Datamodel {
// Any system that wants to listen to the datamodel
// needs to implement this interface and register to be a listener.
class DMListener {
  public:
    virtual void onDatamodelEvent(const DMEvent& event) = 0;
};
void RegisterDatamodelListener(DMListener* listener);

} // namespace Datamodel
} // namespace Engine