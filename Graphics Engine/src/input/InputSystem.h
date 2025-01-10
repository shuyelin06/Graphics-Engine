#pragma once

#include <Windows.h>
#include <vector>

#include "InputData.h"

namespace Engine {
namespace Input {

// InputSystem Class
// Provides a high-level interface for managing
// user input
class InputSystem {
  private:
    // Accumulated input data that has yet to
    // be processed
    std::vector<InputData> inputData;

    // Function callback chain
    std::vector<bool (*)(InputData)> callbackChain;

    // Screen center x and y
    int center_x;
    int center_y;

  public:
    InputSystem();

    // Initialize the input system
    void initialize();

    // Dispatch accumulated input data and evaluate it
    // against the callback chain.
    void update();

    // Handles raw Win32 input. If the input is parsed, returns true. Otherwise,
    // returns false.
    bool handleWin32Input(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
} // namespace Input
} // namespace Engine