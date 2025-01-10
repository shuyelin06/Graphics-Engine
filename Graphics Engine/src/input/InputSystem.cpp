#include "InputSystem.h"

#include "InputCallback.h"

#include "callbacks/InputPoller.h"

namespace Engine {
namespace Input {
// Character table which can be used for keycode conversions
static char CharacterTable[] = {
    // Indices 0 - 9
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    // Indices 10 - 19
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    // Indices 20 - 29
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    // Indices 30 - 35
    'u', 'v', 'w', 'x', 'y', 'z'};

// Constructor
// Initializes the input engine
InputSystem::InputSystem() {
    inputData = std::vector<InputData>();

    // Set screen center at 0
    center_x = center_y = 0;
}

// Initialize:
// Registers the InputPoller callback into the dispatch chain
// for input polling functionality
void InputSystem::initialize() {
    InputCallback::RegisterInputHandler(&InputPoller::UpdateInputStates);
}

// Update:
// Evaluates all accumulated input data against the callback chain.
// Any input data not accepted will remain in the callback chain (in FIFO
// order) for the next dispatch call.
void InputSystem::update() {
    // Unregister all handles stored in the InputCallback static interface
    int head = 0;

    for (int i = 0; i < callbackChain.size(); i++) {
        bool (*func_ptr)(InputData) = callbackChain[i];

        // If handle is to be removed, remove it
        if (std::find(InputCallback::handlesToRemove.begin(),
                      InputCallback::handlesToRemove.end(), callbackChain[i]) !=
            InputCallback::handlesToRemove.end()) {
            callbackChain[i] = nullptr;
        } else {
            callbackChain[head] = callbackChain[i];
            head++;
        }
    }

    callbackChain.resize(head);

    // Register all handles stored in the InputCallback static interface
    callbackChain.insert(std::end(callbackChain),
                         std::begin(InputCallback::handlesToAdd),
                         std::end(InputCallback::handlesToAdd));

    // Clear both vectors in the InputCallback static class
    InputCallback::handlesToAdd.clear();
    InputCallback::handlesToRemove.clear();

    // Stores InputData not processed
    std::vector<InputData> unprocessed = std::vector<InputData>();

    // Iterate through all input data and evaluate each against the
    // callback chain
    for (InputData data : inputData) {
        int callbackIndex = 0;

        // Iterate through callback chain and attempt to evaluate
        // the input data. If "true" is received, we stop
        while (callbackIndex < callbackChain.size() &&
               callbackChain[callbackIndex](data) == false)
            callbackIndex++;

        // If we're at the end of the callback chain, then we failed to evaluate
        // the input data. Register it into the unprocessed vector to remain
        // for the next dispatch() call.
        if (callbackIndex == callbackChain.size())
            unprocessed.push_back(data);
    }

    // Clear accumulated inputData, and only keep unprocessed data
    inputData.clear();
    inputData.insert(std::end(inputData), std::begin(unprocessed),
                     std::end(unprocessed));
}

// LogWin32Input:
// Accepts Win32 raw input messages and converts them into an
// input format usable by the rest of the engine
static char ConvertWin32Keycode(WPARAM wParam);

bool InputSystem::handleWin32Input(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam) {
    // Attempt to find a suitable data format "INVALID"
    InputData data;
    data.input_type = INVALID;

    // Check the type of message being received
    // and attempt to convert into an input data type
    switch (uMsg) {
    case WM_KEYDOWN: {
        char key = ConvertWin32Keycode(wParam);

        if (key != 0) {
            data.input_type = SYMBOL_DOWN;
            data.symbol = key;
        }
    } break;

    case WM_KEYUP: {
        char key = ConvertWin32Keycode(wParam);

        if (key != 0) {
            data.input_type = SYMBOL_UP;
            data.symbol = key;
        }
    } break;
    }

    // If a suitable input conversion was performed, add to
    // accumulated input data for later dispatch
    if (data.input_type != INVALID) {
        inputData.push_back(data);
        return true;
    }
    else 
        return false;
        
}

// Static helper which will convert a Win32 keycode into a
// character suitable for the engine. Returns 0 if unable to convert.
// Converts based on
// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
static char ConvertWin32Keycode(WPARAM wParam) {
    char output = 0;

    int keyCode = wParam;

    // 0 - 9 Key Range
    if (0x30 <= keyCode && keyCode <= 0x39) {
        output = CharacterTable[keyCode - 0x30];
    }
    // A - Z Key Range
    else if (0x41 <= keyCode && keyCode <= 0x5A) {
        output = CharacterTable[keyCode - 0x41 + 10];
    }

    return output;
}

} // namespace Input
} // namespace Engine