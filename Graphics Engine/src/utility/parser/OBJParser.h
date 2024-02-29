#pragma once

// DEBUG OUTPUT


#include <string>

namespace Engine 
{
namespace Utility
{

	// Class OBJParser
	// Basic parser for .obj files
	class OBJParser
	{
	public:
		static void parseFile(std::string obj_file);
	};
}
}