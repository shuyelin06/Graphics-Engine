#pragma once

namespace Engine
{
namespace Input
{
	// Input data types that are supported 
	typedef enum { 
		INVALID,
		SYMBOL_DOWN, SYMBOL_UP, 
		DEVICE_MOVE_X, DEVICE_MOVE_Y, DEVICE_INTERACT
	} InputType;
	
	// InputData Class:
	// Represents various input data
	class InputData
	{
	public:
		// Denotes the type of input data being stored
		InputType input_type;
		
		// Information about the symbol, if the type is related to symbols
		char symbol;

		// Information about the X/Y movement, if the type is related to devices
		float movement;
	};
}
}