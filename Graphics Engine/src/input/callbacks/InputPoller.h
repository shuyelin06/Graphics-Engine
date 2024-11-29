#pragma once

#include "input/InputCallback.h"

namespace Engine {
namespace Input {
// InputPoller Class:
// Uses callback functions to internally store the state
// of various inputs in the engine. Can be used by
// other systems to poll input on the fly.
class InputPoller {
  private:
    // Stores the key-state of symbols 0-9, a-z
    // (in that order)
    static bool symbol_state[10 + 26];

  public:
    // Return the key-state of some symbol
    static bool IsSymbolActive(char symbol);

    // Callback function which will update the poller's data
    static bool UpdateInputStates(InputData inputData);
};
} // namespace Input
} // namespace Engine