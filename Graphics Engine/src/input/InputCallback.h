#pragma once

#include <vector>

#include "InputData.h"

namespace Engine
{
namespace Input
{
	// Input Callback Class:
	// Class containing static methods which are the main
	// interface to register (or de-register) input
	// handlers created elsewhere in the Engine.
	class InputCallback
	{
	private:
		friend class InputEngine;

		static std::vector<bool (*)(InputData)> handlesToAdd;
		static std::vector<bool (*)(InputData)> handlesToRemove;
	
	public:
		// Registers a callback function which can handle input.
		// Callback functions are expected to return true when they
		// handle the data, and false otherwise.
		static void RegisterInputHandler(bool (*handle)(InputData));

		// Unregisters a callback function
		static void RemoveInputHandler(bool (*handle)(InputData));
	};
}
}