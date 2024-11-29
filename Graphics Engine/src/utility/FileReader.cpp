#include "FileReader.h"

#include <iostream>
#include <stdio.h>

namespace Engine {
namespace Utility {
// Represents a substring of the source data [begin, end),
// which is treated as a "block" of data.
struct BlockInterval {
    int begin;
    int end;

    BlockInterval() : begin(-1), end(-1) {}

    BlockInterval(int _begin, int _end) : begin(_begin), end(_end) {}
};

// Constructor:
// Initializes the file reader and opens a stream.
TextFileReader::TextFileReader(const std::string& fileName)
    : inputStream(fileName), sourceData(), blocks(0) {}

// Destructor:
// Closes the input stream.
TextFileReader::~TextFileReader() { inputStream.close(); }

// ExtractBlock:
// Extracts a block of data from the current block. This is either the most
// recently extracted block, to the next block of input into the file reader.
// Reads until it finds the terminator, or the end of the block (or file) is
// reached. This data is then pushed onto the stack for processing. The block of
// data extracted is removed from the parent it was taken from.
bool TextFileReader::extractBlock(char terminator) {
    bool success = false;

    // If no blocks currently exist, read directly from file until we
    // hit our terminator.
    if (blocks.size() == 0) {
        sourceData.clear();

        // Read from the file until we hit our terminator, or there is no
        // more to read from the file.
        char c;

        while (inputStream.get(c)) {
            success = true;

            if (c != terminator)
                sourceData.push_back(c);
            else
                break;
        }

        // If anything was read, load a block encompassing the entirety of the
        // read data
        if (success) {
            const BlockInterval interval =
                BlockInterval(0, sourceData.length());
            blocks.push_back(interval);
        }
    }
    // Otherwise, read directly from the last block
    else {
        BlockInterval& mostRecentInterval = blocks[blocks.size() - 1];

        // If the most recent block is empty, we cannot extract anything and
        // fail.
        if (mostRecentInterval.begin == mostRecentInterval.end)
            success = false;
        // Otherwise, we extract the substring between the beginning of the
        // block and the terminator (or the end of the block, if terminator does
        // not exist)
        else {
            success = true;

            const int terminatorIndex =
                sourceData.find(terminator, mostRecentInterval.begin);

            // If we could find the terminator within the source block,
            // extract begin -> terminator from the current block
            if (terminatorIndex != std::string::npos &&
                terminatorIndex < mostRecentInterval.end) {
                const BlockInterval interval =
                    BlockInterval(mostRecentInterval.begin, terminatorIndex);
                mostRecentInterval.begin = terminatorIndex + 1;
                blocks.push_back(interval);
            }
            // Otherwise, extract the entirety of the most recent block.
            else {
                const BlockInterval interval = BlockInterval(
                    mostRecentInterval.begin, mostRecentInterval.end);
                mostRecentInterval.begin = mostRecentInterval.end;
                blocks.push_back(interval);
            }
        }
    }

    return success;
}

// ViewBlock:
// Returns the currently loaded block of the text file reader.
std::string TextFileReader::viewBlock() const {
    if (blocks.size() > 0) {
        const BlockInterval interval = blocks[blocks.size() - 1];
        return sourceData.substr(interval.begin, interval.end);
    } else
        return 0;
}

// PopBlock:
// Removes the most recently loaded block of the text file reader.
bool TextFileReader::popBlock() {
    if (blocks.size() > 0) {
        blocks.pop_back();
        return true;
    } else
        return false;
}

// TrimBlockFront:
// Removes n occurrences of the character c from the front of the block.
int TextFileReader::lstripBlock(char c, int n) {
    BlockInterval& mostRecentInterval = blocks[blocks.size() - 1];

    int numStripped = 0;
    while (mostRecentInterval.begin < mostRecentInterval.end &&
           (n == -1 || numStripped < n) &&
           sourceData[mostRecentInterval.begin] == c) {
        mostRecentInterval.begin++;
        numStripped++;
    }

    return numStripped;
}

// Parsers:
// Tries to parse the most recent block as a certain data type.
// Returns true on success, and loads result. On fail, does nothing and returns
// false.
bool TextFileReader::parseAsFloat(float* result) {
    if (blocks.size() > 0) {
        const BlockInterval recentInterval = blocks[blocks.size() - 1];
        const std::string str =
            sourceData.substr(recentInterval.begin, recentInterval.end);
        float parsed = (float)std::stof(str.c_str());
        *result = parsed;
        return true;
    } else
        return false;
}

bool TextFileReader::parseAsInt(int* result) {
    if (blocks.size() > 0) {
        const BlockInterval recentInterval = blocks[blocks.size() - 1];
        if (recentInterval.begin == recentInterval.end)
            return false;
        else {
            const std::string str =
                sourceData.substr(recentInterval.begin, recentInterval.end);
            int parsed = (int)std::atoi(str.c_str());
            *result = parsed;
            return true;
        }
    } else
        return false;
}

// Readers:
// Automatically tries to extract, parse, and pop blocks of data to
// load a specified datatype. Does this for convenience, though it should only
// be used in cases where you're sure what comes next.
bool TextFileReader::readString(std::string* result, char terminator) {
    if (extractBlock(terminator)) {
        *result = viewBlock();
        popBlock();
        return true;
    } else
        return false;
}

bool TextFileReader::readFloat(float* result, char terminator) {
    if (extractBlock(terminator)) {
        bool status = parseAsFloat(result);
        popBlock();
        return status;
    } else
        return false;
}

bool TextFileReader::readInt(int* result, char terminator) {
    if (extractBlock(terminator)) {
        bool status = parseAsInt(result);
        popBlock();
        return status;
    } else
        return false;
}
} // namespace Utility
} // namespace Engine