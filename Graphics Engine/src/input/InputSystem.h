#pragma once

#include <Windows.h>
#include <list>
#include <vector>

#include "EventHandler.h"
#include "InputState.h"

namespace Engine {
namespace Input {

// InputSystem Class
// Provides a high-level interface for managing
// user input
class InputSystem {
  private:
    // Event callback chains
    std::vector<EventHandle> callback_chains[EventCount];

    // Window X,Y (Pixel) Screen Dimensions
    int window_width, window_height;

  public:
    InputSystem();

    // Initialize the input system
    void initialize(HWND hwnd);

    // Dispatch accumulated input data and evaluate it
    // against the callback chain.
    void update();

    // Handles raw Win32 input. If the input is parsed, returns true. Otherwise,
    // returns false.
    bool dispatchWin32Input(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  private:
    bool dispatchEvent(InputEvent event, const EventData& data);

#if defined(_DEBUG)
  private:
    void imGuiDisplay();
#endif
};
} // namespace Input
} // namespace Engine