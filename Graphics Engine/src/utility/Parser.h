#pragma once

#include <string>

namespace Engine
{
namespace Utility
{
	// Parser
	// Contains various static parsers which can
	// be used to read files
	class Parser
	{
		static void parsePLYFile(std::string ply_file);
	};

}
}