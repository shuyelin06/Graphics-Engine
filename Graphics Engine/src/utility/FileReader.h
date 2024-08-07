#pragma once

#include <fstream>
#include <string>

#include <vector>

#define LSTRIP_INFINITE -1

namespace Engine
{
namespace Utility
{
	// TextFileReader Class:
	// Helper class that can be used to more conveniently
	// read files for data. Assumes text input.
	// The way the reading works is as follows:
	// 1) Read a "block" of data with the end of the block specified by a delimiter.
	//    If a block already exists, uses the most recently extracted block as the data.
	//    Any extracted data is removed from its source.
	// 2) Process this block of data using the reader's built-in functionalities.
	// 3) Pop this block.
	struct BlockInterval;
	class TextFileReader
	{
	private:
		std::ifstream inputStream;

		// Current data from the input stream
		std::string sourceData;

		// Stack of blocks of our currently read data. All blocks reference the same source
		// data, but can reference different substring indices of this data.
		std::vector<BlockInterval> blocks;

	public:
		TextFileReader(const std::string& fileName);
		~TextFileReader();

		// Extracts a block of data from the most recent block (or if none, the file), terminating 
		// at the character specified, or until the end of the existing block.
		// Removes any extracted data from its parent, dropping the terminator. 
		// Returns true if the parent block was modified, false otherwise.
		bool extractBlock(char terminator);
		
		// View the most recently extracted block
		std::string viewBlock() const;
		
		// Pops the most recently extracted block.
		bool popBlock();

		// --- Block Operations ---
		// Strips n occurrences of the character off the left end of 
		// the current block, or until there is no more of the character.
		// Returns the # of characters stripped.
		// Pass in -1 for n if we don't want to set a limit on the number of characters
		// to strip.
		int lstripBlock(char c, int n);

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