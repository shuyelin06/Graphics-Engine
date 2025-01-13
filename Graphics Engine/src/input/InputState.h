#pragma once

#include <bitset>

#include "SymbolData.h"

namespace Engine {
namespace Input {

// InputPoller Class:
// Internally stores the state of various inputs in the engine.
// Updated by the InputSystem.
// Can be used by other systems to poll input on the fly.
class InputState {
  private:
    friend class InputSystem;

    // Stores the key-state of symbols.
    // See SymbolData.h for the list of symbols tracked
    static std::bitset<SymbolCount> symbol_state;
    
    // Stores the device position in screen space coordinates
    // spanning [0,1]. x goes left to right, y goes bottom to top.
    static float device_x, device_y;

    // Callback function which will update the poller's data
    static void SetInputSymbolActive(InputSymbol symbol);
    static void SetInputSymbolInactive(InputSymbol symbol);
    static void SetDeviceCoordinates(float x, float y);

  public:
    // Return the key-state of some symbol
    static bool IsSymbolActive(InputSymbol symbol);
    // Return device coordinates
    static float DeviceXCoordinate();
    static float DeviceYCoordinate();
};
} // namespace Input
} // namespace Engine