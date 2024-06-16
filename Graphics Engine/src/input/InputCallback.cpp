#include "InputCallback.h"

namespace Engine
{
namespace Input
{
	// Static variable initialization
	std::vector<bool (*)(InputData)> InputCallback::handlesToAdd = std::vector<bool (*)(InputData)>();
	std::vector<bool (*)(InputData)> InputCallback::handlesToRemove = std::vector<bool (*)(InputData)>();

	// RegisterInputHandler:
	// Main interface for registering an input handler.
	// Note that an input handler must accept InputData as an argument, and return a
	// boolean indicating if the input was consumed or not by this handle.
	void InputCallback::RegisterInputHandler(bool (*handle)(InputData))
	{
		handlesToAdd.push_back(handle);
	}

	// RemoveInputHandler:
	// Main interface for removing an input handler.
	// Note to successfully remove a handle, the same function pointer should be used.
	void InputCallback::RemoveInputHandler(bool (*handle)(InputData))
	{
		handlesToRemove.push_back(handle);
	}
}
}