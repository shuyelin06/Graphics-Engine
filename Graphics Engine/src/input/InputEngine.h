#pragma once

#include <vector>
#include <Windows.h>

#include "InputData.h"

#include "datamodel/Scene.h"

namespace Engine
{
namespace Input
{

	// InputEngine Class
	// Provides a high-level interface
	// for managing user input
	class InputEngine 
	{
	private:
		// Accumulated input data that has yet to
		// be processed
		std::vector<InputData> inputData;

		// Function callback chain
		std::vector<bool (*)(InputData)> callbackChain;

		// Scene
		Datamodel::Scene* scene;

		// Screen center x and y
		int center_x;
		int center_y;

	public:
		InputEngine();
		
		// Convert raw Win32 input into a format suitable
		// for the input engine
		void logWin32Input(UINT uMsg, WPARAM wParam);

		// Dispatch accumulated input data and evaluate it
		// against the callback chain.
		void dispatch();

		// Set Scene
		void setScene(Datamodel::Scene* scene);

		// Set screen center, to lock the mouse
		void setScreenCenter(int center_x, int center_y);

		// Update camera
		void updateCameraView();

		// Handle Key Down Input
		void handleKeyDown(int key);

		// Handle Key Up Input
		void handleKeyUp(int key);
	};
}
}