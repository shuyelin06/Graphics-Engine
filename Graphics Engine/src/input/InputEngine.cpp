#include "InputEngine.h"

#include <Windows.h>

namespace Engine
{
using namespace Datamodel;

namespace Input
{
	// Constructor
	InputEngine::InputEngine() 
	{
		// Set screen center at 0
		center_x = center_y = 0;
	}

	// Set screen center
	void InputEngine::setScreenCenter(int _center_x, int _center_y) 
	{
		center_x = _center_x;
		center_y = _center_y;
	}

	// Handle camera movement with the mouse
	void InputEngine::updateCameraView(Camera* camera)
	{
		// Get new mouse position
		POINT new_pos;
		GetCursorPos(&new_pos);
		
		// Determine mouse displacement
		int x_delta = new_pos.x - center_x;
		int y_delta = new_pos.y - center_y;

		// Convert to Angular Displacement
		// Roll = Rotation Around X (Up/Down)
		// Pitch = Rotation Around Y (Left/Right
		float roll_delta = y_delta / 100.f;
		float pitch_delta = x_delta / 100.f;

		camera->offsetRotation(roll_delta, pitch_delta, 0);
		
		// Reset mouse to center of application
		SetCursorPos(center_x, center_y);
	}

	// Handle Key Down Events
	void InputEngine::handleKeyDown(Player* player, int key)
	{
		switch (key)
		{
		// ESCAPE: Exit the application
		case VK_ESCAPE: // ESCAPE
		{
			ClipCursor(NULL);
			PostQuitMessage(0);
		}
		break;

		case 0x57: // W
		{
			Camera* camera = player->getCamera();
			Vector3 forward_view = camera->forward();

			player->offsetVelocity(forward_view * 10.f);
		}	
		break;

		case 0x53: // S
		{
			Camera* camera = player->getCamera();
			Vector3 backward_view = -camera->forward();

			player->offsetVelocity(backward_view * 10.f);
		}
		break;

		case 0x41: // A
		{
			Camera* camera = player->getCamera();
			Vector3 left_view = -camera->right();

			player->offsetVelocity(left_view * 10.f);
		}
		break;

		case 0x44: // D
		{
			Camera* camera = player->getCamera();
			Vector3 right_view = camera->right();

			player->offsetVelocity(right_view * 10.f);
		}
		break;

		}
	}

	// handle Key Up Events
	void InputEngine::handleKeyUp(int key)
	{
		switch (key)
		{
		case 0x57: // W
			
			break;

		case 0x53: // S
			
			break;

		case 0x41: // A
			
			break;

		case 0x44: // D
			
			break;
		}
	}
}
}