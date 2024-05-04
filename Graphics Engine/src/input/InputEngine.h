#pragma once

#include <Windows.h>

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
		// Scene
		Datamodel::Scene* scene;

		// Screen center x and y
		int center_x;
		int center_y;

	public:
		InputEngine();
		
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