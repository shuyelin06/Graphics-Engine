#include "InputEngine.h"

#include <Windows.h>

namespace Engine
{
namespace Input
{
	// Constructor
	InputEngine::InputEngine() {
		for (int i = 0; i < 0xFF; i++)
		{
			down_handles[i] = nullptr;
			up_handles[i] = nullptr;
		}
	}

	// Call some function by input
	void InputEngine::handleInput()
	{
		for (int key = 1; key < 0XFF; key++)
		{
			// Checks the highest bit for a key down, and if
			// it is set, then the key is pressed
			int keydown = (1 << 15) & GetAsyncKeyState(key);

			// If key is pressed, run down handle (if it exists)
			if (keydown)
			{
				if (down_handles[key] != nullptr)
					down_handles[key]();
			}
			// Otherwise, run up handle (if it exists)
			else
			{
				if (up_handles[key] != nullptr)
					up_handles[key]();
			}
		}
	}

	// Bind some function to a key down press
	void InputEngine::bindKeyDown(int keyval, void (*func)(void))
	{
		// Assign function to associated key
		down_handles[keyval] = func;
	}

	// Bind some function to a key up press
	void InputEngine::bindKeyUp(int keyval, void (*func)(void))
	{
		// Assign function to associated key
		up_handles[keyval] = func;
	}

	// Remove down function-key binding
	void InputEngine::removeKeyDown(int keyval)
	{
		// Remove function associated with that key
		down_handles[keyval] = nullptr;
	}

	// Remove up function-key binding
	void InputEngine::removeKeyUp(int keyval)
	{
		// Remove function associated with that key
		up_handles[keyval] = nullptr;
	}
}
}