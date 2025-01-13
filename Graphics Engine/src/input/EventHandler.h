#pragma once

#include <vector>

#include "EventData.h"

namespace Engine {
namespace Input {

typedef bool (*EventHandle)(const EventData&);

struct HandleData {
    InputEvent event_type;
    EventHandle handle;
};
// Input Callback Class:
// Class containing static methods which are the main
// interface to register (or de-register) input
// handlers created elsewhere in the Engine.
class EventHandler {
  private:
    friend class InputSystem;

    static std::vector<HandleData> handlesToAdd;
    static std::vector<HandleData> handlesToRemove;

  public:
    // Registers a callback function which can handle input.
    // Callback functions are expected to return true when they
    // handle the data, and false otherwise.
    static void RegisterEventHandler(InputEvent event_type,
                                     EventHandle handle);

    // Unregisters a callback function
    static void RemoveEventHandler(InputEvent event_type,
                                   EventHandle handle);
};
} // namespace Input
} // namespace Engine