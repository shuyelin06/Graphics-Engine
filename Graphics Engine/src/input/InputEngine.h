#pragma once

#include "datamodel/Camera.h"
#include "datamodel/other/Player.h"

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
		// Screen center x and y
		int center_x;
		int center_y;

	public:
		InputEngine();
		
		// Set screen center, to lock the mouse
		void setScreenCenter(int center_x, int center_y);

		// Update camera
		void updateCameraView(Datamodel::Camera*);

		// Handle Key Down Input
		void handleKeyDown(Datamodel::Player*, int key);

		// Handle Key Up Input
		void handleKeyUp(int key);
	};
}
}