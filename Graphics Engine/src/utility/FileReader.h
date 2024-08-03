#pragma once

#include <fstream>
#include <string>

#include <vector>

namespace Engine
{
namespace Utility
{
	// TextFileReader Class:
	// Helper class that can be used to more conveniently
	// read files for data. Assumes text input.
	// The way the reading works is as follows:
	// 1) Read a "block" of data from the file, with the end of the block
	//    specified by a delimiter.
	// 2) Process this block of data using the reader's built-in functionalities.
	class TextFileReader
	{
	private:
		std::ifstream inputStream;

		// Stack of currently read blocks. Each one is a subset
		// of the previous (with the first being a subset of the file data)
		std::vector<std::string> blocks;

	public:
		TextFileReader(const std::string& fileName);
		~TextFileReader();

		// Extracts a block of data from the existing block (or if none, the file), 
		// terminating at the character specified. Removes this block from its parent.
		bool extractBlock(char terminator);
		const std::string& viewBlock() const;
		bool popBlock();

		// Attempt to parse the current block into a differnt format
		bool parseAsFloat(float* result);
		bool parseAsInt(int* result);
		
		// Helper methods that will automatically extract, parse,
		// and pop a block for specific data. Performs this on the current block.
		bool readString(std::string* result, char terminator);
		bool readFloat(float* result, char terminator);
		bool readInt(int* result, char terminator);
	};
}
}