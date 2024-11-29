#include "InputPoller.h"

namespace Engine {
namespace Input {
// Initialize the internal symbol_state array
bool InputPoller::symbol_state[36] = {false};

// Internally used to index the symbols array.
static int IndexSymbol(char symbol);

// IsSymbolActive:
// Returns true if the symbol is currently active (pressed), and false
// otherwise.
bool InputPoller::IsSymbolActive(char symbol) {
    int index = IndexSymbol(symbol);

    if (index != -1)
        return symbol_state[index];
    else
        return false;
}

// UpdateInputStates:
// Callback function which will be inserted into the input
// dispatch chain, to intercept input and update the symbol
// state array as needed
bool InputPoller::UpdateInputStates(InputData inputData) {
    // Update the symbol array if SYMBOL_DOWN or SYMBOL_UP
    // is intercepted
    if (inputData.input_type == SYMBOL_DOWN ||
        inputData.input_type == SYMBOL_UP) {
        // If the symbol exists in the array, update its state.
        int index = IndexSymbol(inputData.symbol);

        if (index != -1) {
            bool new_state = inputData.input_type == SYMBOL_DOWN ? true : false;
            symbol_state[index] = new_state;
        }
    }

    // Always returns false, so that the data will pass through the
    // rest of the input chain
    return false;
}

// IndexSymbol:
// Static helper method which maps symbols to indices in the
// symbol array
static int IndexSymbol(char symbol) {
    // Symbols 0 - 9 map to indices 0 to 9, respectively
    if ('0' <= symbol && symbol <= '9') {
        return symbol - '0';
    }
    // Symbols a - z map to indices 10 to 35, respectively
    else if ('a' <= symbol && symbol <= 'z') {
        return symbol - 'a' + 10;
    }
    // Otherwise, no mapping exists, so return -1
    else
        return -1;
}

} // namespace Input
} // namespace Engine