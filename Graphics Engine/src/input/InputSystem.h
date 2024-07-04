#pragma once

#include <Windows.h>
#include <vector>

#include "input/components/MovementComponent.h"

#include "InputData.h"

namespace Engine
{
namespace Input
{

	// InputSystem Class
	// Provides a high-level interface for managing 
	// user input
	class InputSystem
	{
	private:
		// Components
		std::vector<MovementComponent*> movement_components;

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
		
		// Convert raw Win32 input into a format suitable
		// for the input engine
		void logWin32Input(UINT uMsg, WPARAM wParam);

		// Create movement component
		MovementComponent* bindMovementComponent(Datamodel::Object* object);
		bool removeMovementComponent(MovementComponent* component);

	};
}
}