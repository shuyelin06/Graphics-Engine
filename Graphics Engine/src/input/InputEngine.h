#pragma once

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
		// Arrays of void function pointers
		void (*down_handles[0xFF])(void); // Runs when key is pressed down 
		void (*up_handles[0xFF])(void); // Runs when key is not pressed down
	
	public:
		InputEngine();

		// Handle input for all keys at once. Call in the main engine loop
		void handleInput(void);

		// Handle key bindings
		void bindKeyDown(int keyval, void (*func)(void)); // Add some function as input
		void bindKeyUp(int keyval, void (*func)(void));

		void removeKeyDown(int keyval); // Remove key down binding
		void removeKeyUp(int keyval);
	};
}
}