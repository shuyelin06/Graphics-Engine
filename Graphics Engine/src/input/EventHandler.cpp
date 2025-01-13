#include "EventHandler.h"

namespace Engine {
namespace Input {
std::vector<HandleData> EventHandler::handlesToAdd = std::vector<HandleData>();
std::vector<HandleData> EventHandler::handlesToRemove =
    std::vector<HandleData>();

// RegisterInputHandler:
// Main interface for registering an input handler.
// Note that an input handler must accept InputData as an argument, and return a
// boolean indicating if the input was consumed or not by this handle.
void EventHandler::RegisterEventHandler(InputEvent event_type,
                                        EventHandle handle) {
    HandleData data;
    data.event_type = event_type;
    data.handle = handle;
    handlesToAdd.push_back(data);
}

// RemoveInputHandler:
// Main interface for removing an input handler.
// Note to successfully remove a handle, the same function pointer should be
// used.
void EventHandler::RemoveEventHandler(InputEvent event_type,
                                      EventHandle handle) {
    HandleData data;
    data.event_type = event_type;
    data.handle = handle;
    handlesToRemove.push_back(data);
}
} // namespace Input
} // namespace Engine