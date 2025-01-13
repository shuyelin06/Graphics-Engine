#include "InputState.h"

namespace Engine {
namespace Input {
std::bitset<SymbolCount> InputState::symbol_state = {false};
float InputState::device_x = 0;
float InputState::device_y = 0;

// Internally used to index the symbols array.
static int IndexSymbol(char symbol);

// Accessors:
// Return status of symbols and device
bool InputState::IsSymbolActive(InputSymbol symbol) {
    return symbol_state[symbol];
}
float InputState::DeviceXCoordinate() { return device_x; }
float InputState::DeviceYCoordinate() { return device_y; }

// Updates:
// Different functions that the InputSystem can use to
// internally update the InputState data.
void InputState::SetInputSymbolActive(InputSymbol symbol) {
    symbol_state[symbol] = true;
}
void InputState::SetInputSymbolInactive(InputSymbol symbol) {
    symbol_state[symbol] = false;
}
void InputState::SetDeviceCoordinates(float x, float y) {
    device_x = x;
    device_y = y;
}

} // namespace Input
} // namespace Engine