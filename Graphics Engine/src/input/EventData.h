#pragma once

#include "SymbolData.h"

namespace Engine {
namespace Input {

// Input Events
enum InputEvent {
    SYMBOL_PRESSED,
    DEVICE_INTERACTION,
    EventCount,
    EVENT_INVALID
};

// Event Data:
// Extra data for a given input event
struct EventSymbolPressed {
    InputSymbol symbol;
};
struct EventDeviceInteraction {
    float device_x, device_y;
};

// InputData Class:
// Represents various input data
struct EventData {
    union {
        EventSymbolPressed symbol_pressed;
        EventDeviceInteraction device_interaction;
    };
};

} // namespace Input
} // namespace Engine