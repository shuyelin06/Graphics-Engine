#include "InputSystem.h"

#include <WindowsX.h> // Input Macros

#include "EventHandler.h"
#include "InputState.h"

#if defined(_DEBUG)
#include "rendering/ImGui.h"

#define DISPLAY_DEVICE_STATE
#define DISPLAY_SYMBOL_STATE
#endif

namespace Engine {
namespace Input {

// Constructor
// Initializes the input engine
InputSystem::InputSystem(HWND hwnd) {
    RECT rect;
    GetWindowRect(hwnd, &rect);
    window_width = rect.right - rect.left;
    window_height = rect.bottom - rect.top;

    // EventHandler::RegisterInputHandler(&InputPoller::UpdateInputStates);
}

// Update:
// Evaluates all accumulated input data against the callback chain.
// Any input data not accepted will remain in the callback chain (in FIFO
// order) for the next dispatch call.
void InputSystem::update() {
    // Evaluate Event Handles
    for (const HandleData& data : EventHandler::handlesToAdd) {
        callback_chains[data.event_type].push_back(data.handle);
    }

    for (const HandleData& data : EventHandler::handlesToRemove) {
        std::vector<EventHandle> chain = callback_chains[data.event_type];

        auto iter = std::find(chain.begin(), chain.end(), data.handle);
        if (iter != chain.end())
            chain.erase(iter);
    }

    EventHandler::handlesToAdd.clear();
    EventHandler::handlesToRemove.clear();

    // Poll the InputState and Send Events as Necessary
    // --- Symbol Pressed Events
    EventData data;

    for (int i = 0; i < SymbolCount; i++) {
        const InputSymbol symbol = static_cast<InputSymbol>(i);
        if (InputState::IsSymbolActive(symbol)) {
            data.symbol_pressed.symbol = symbol;
            dispatchEvent(SYMBOL_PRESSED, data);
        }
    }

    if (InputState::IsSymbolActive(DEVICE_INTERACT) ||
        InputState::IsSymbolActive(DEVICE_ALT_INTERACT)) {
        data.device_interaction.device_x = InputState::DeviceXCoordinate();
        data.device_interaction.device_y = InputState::DeviceYCoordinate();
        dispatchEvent(DEVICE_INTERACTION, data);
    }

#if defined(_DEBUG)
    imGuiDisplay();
#endif
}

// DispatchEvent:
// Dispatch event information to a callback chain. Return true if a function
// returned true (meaning it processed the event), false otherwise.
bool InputSystem::dispatchEvent(InputEvent event, const EventData& data) {
    std::vector<EventHandle> callback_chain = callback_chains[event];

    bool processed = false;
    int i = callback_chain.size() - 1;

    while (!processed && i >= 0) {
        EventHandle handle = callback_chain[i];

        if ((*handle)(data))
            processed = true;
        else
            i--;
    }

    return processed;
}

// LogWin32Input:
// Accepts Win32 raw input messages and converts them into an
// input format usable by the rest of the engine
static InputSymbol ConvertWin32Keycode(WPARAM wParam);

bool InputSystem::dispatchWin32Input(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam) {
    bool parsed = true;

    // Check the type of message being received
    // and attempt to parse it
    switch (uMsg) {
    case WM_KEYDOWN: {
        const InputSymbol key = ConvertWin32Keycode(wParam);
        if (key != SYMBOL_INVALID)
            InputState::SetInputSymbolActive(key);
    } break;

    case WM_KEYUP: {
        InputSymbol key = ConvertWin32Keycode(wParam);
        if (key != SYMBOL_INVALID)
            InputState::SetInputSymbolInactive(key);
    } break;

    case WM_LBUTTONDOWN:
        InputState::SetInputSymbolActive(DEVICE_INTERACT);
        break;
    case WM_LBUTTONUP:
        InputState::SetInputSymbolInactive(DEVICE_INTERACT);
        break;
    case WM_RBUTTONDOWN:
        InputState::SetInputSymbolActive(DEVICE_ALT_INTERACT);
        break;
    case WM_RBUTTONUP:
        InputState::SetInputSymbolInactive(DEVICE_ALT_INTERACT);
        break;

    case WM_MOUSEMOVE: {
        const int x_pos = GET_X_LPARAM(lParam);
        const int y_pos = GET_Y_LPARAM(lParam);

        const float screen_x = float(x_pos) / float(window_width);
        const float screen_y =
            float(window_height - y_pos) / float(window_height);

        InputState::SetDeviceCoordinates(screen_x, screen_y);
    } break;

    default:
        parsed = false;
        break;
    }

    return parsed;
}

// Static helper which will convert a Win32 keycode into a
// character suitable for the engine. Returns 0 if unable to convert.
// Converts based on
// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
static InputSymbol ConvertWin32Keycode(WPARAM wParam) {
    InputSymbol output = SYMBOL_INVALID;

    int keyCode = wParam;

    // 0 - 9 Key Range
    if (0x30 <= keyCode && keyCode <= 0x39) {
        output = static_cast<InputSymbol>(keyCode - 0x30 + NUM_0);
    }
    // A - Z Key Range
    else if (0x41 <= keyCode && keyCode <= 0x5A) {
        output = static_cast<InputSymbol>(keyCode - 0x41 + KEY_A);
    }
    // Extra Misc. Keys
    else {
        switch (keyCode) {
        case VK_CONTROL:
            output = KEY_CONTROL;
            break;

        case VK_SHIFT:
            output = KEY_SHIFT;
            break;

        case VK_LBUTTON:
            output = DEVICE_INTERACT;
            break;
        case VK_RBUTTON:
            output = DEVICE_ALT_INTERACT;
            break;
        }
    }

    return output;
}

#if defined(_DEBUG)
static const std::string SymbolStrings[SymbolCount] = {
    // Numbers: Indices 0 - 9
    "NUM_0", "NUM_1", "NUM_2", "NUM_3", "NUM_4", "NUM_5", "NUM_6", "NUM_7",
    "NUM_8", "NUM_9",
    // Letters: Indices 10 - 35
    "KEY_A", "KEY_B", "KEY_C", "KEY_D", "KEY_E", "KEY_F", "KEY_G", "KEY_H",
    "KEY_I", "KEY_J", "KEY_K", "KEY_L", "KEY_M", "KEY_N", "KEY_O", "KEY_P",
    "KEY_Q", "KEY_R", "KEY_S", "KEY_T", "KEY_U", "KEY_V", "KEY_W", "KEY_X",
    "KEY_Y", "KEY_Z",
    // Misc Keys
    "KEY_CONTROL", "KEY_SHIFT",
    // Device:
    "DEVICE_INTERACT", "DEVICE_ALT_INTERACT"};

// ImGui Display:
// Display input state in the ImGui system
void InputSystem::imGuiDisplay() {
    if (ImGui::CollapsingHeader("Input")) {
#if defined(DISPLAY_DEVICE_STATE)
        ImGui::SeparatorText("Device Info:");
        ImGui::Text("Device x: %f", InputState::device_x);
        ImGui::Text("Device y: %f", InputState::device_y);
#endif

#if defined(DISPLAY_SYMBOL_STATE)
        ImGui::SeparatorText("Symbol Info:");
        for (int i = 0; i < SymbolCount; i++) {
            InputSymbol symbol = static_cast<InputSymbol>(i);
            ImGui::Text("Symbol %s: %d", SymbolStrings[i].c_str(),
                        InputState::IsSymbolActive(symbol));
        }
#endif
    }
}
#endif

} // namespace Input
} // namespace Engine