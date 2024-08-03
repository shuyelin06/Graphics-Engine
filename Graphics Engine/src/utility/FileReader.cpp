#include "FileReader.h"

#include <iostream>
#include <stdio.h>

namespace Engine
{
namespace Utility
{
	// Constructor:
	// Initializes the file reader and opens a stream.
	TextFileReader::TextFileReader(const std::string& fileName)
		: inputStream(fileName)
		, blocks()
	{}

	// Destructor:
	// Closes the input stream.
	TextFileReader::~TextFileReader()
	{
		inputStream.close();
	}

	// ExtractBlock:
	// Extracts a block of data from the current block. This is either the most recently extracted
	// block, to the next block of input into the file reader.
	// Reads until it finds the terminator, or the end of the block (or file) is reached.
	// This data is then pushed onto the stack for processing. 
	// The block of data extracted is removed from the parent it was taken from.
	bool TextFileReader::extractBlock(char terminator)
	{
		// If no blocks currently exist, read directly from file until we
		// hit our terminator.
		if (blocks.size() == 0)
		{
			std::string block;

			char c;
			while (inputStream.get(c) && c != terminator)
				block.push_back(c);

			// Check if anything was read.
			if (block.length() != 0)
			{
				blocks.push_back(block);
				return true;
			}
			else
				return false;
		}
		// Otherwise, read directly from the last block
		else
		{
			const int terminatorIndex = blocks[blocks.size() - 1].find(terminator, 0);

			// Check if we found the terminator. If not, the extraction failed.
			if (terminatorIndex != std::string::npos)
			{
				const std::string extractedBlock = blocks[blocks.size() - 1].substr(0, terminatorIndex);
				blocks[blocks.size() - 1] = blocks[blocks.size() - 1].substr(terminatorIndex + 1);
				blocks.push_back(extractedBlock);
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	// ViewBlock:
	// Returns the currently loaded block of the text file reader.
	const std::string& TextFileReader::viewBlock() const
	{
		return blocks[blocks.size() - 1];
	}

	// PopBlock:
	// Removes the most recently loaded block of the text file reader.
	bool TextFileReader::popBlock()
	{
		if (blocks.size() > 0)
		{
			blocks.pop_back();
			return true;
		}
		else
			return false;
	}

	// Parsers:
	// Tries to parse the most recent block as a certain data type. 
	// Returns true on success, and loads result. On fail, does nothing and returns false.
	bool TextFileReader::parseAsFloat(float* result)
	{
		if (blocks.size() > 0)
		{
			const std::string& block = blocks[blocks.size() - 1];
			float parsed = (float) std::stof(block.c_str());
			*result = parsed;
			return true;
		}
		else
			return false;
	}

	bool TextFileReader::parseAsInt(int* result)
	{
		if (blocks.size() > 0)
		{
			const std::string& block = blocks[blocks.size() - 1];
			int parsed = (int) std::stoi(block.c_str());
			*result = parsed;
			return true;
		}
		else
			return false;
	}

	// Readers:
	// Automatically tries to extract, parse, and pop blocks of data to
	// load a specified datatype. Does this for convenience, though it should only be used
	// in cases where you're sure what comes next.
	bool TextFileReader::readString(std::string* result, char terminator)
	{
		extractBlock(terminator);
		*result = viewBlock();
		popBlock();
		return true;
	}

	bool TextFileReader::readFloat(float* result, char terminator)
	{
		extractBlock(terminator); 
		bool status = parseAsFloat(result);
		popBlock();
		return status;		
	}
	
	bool TextFileReader::readInt(int* result, char terminator)
	{
		extractBlock(terminator);
		bool status = parseAsInt(result);
		popBlock();
		return status;
	}
}
}