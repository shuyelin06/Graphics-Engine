#pragma once

#include <string>
#include <vector>

namespace Engine {
namespace Graphics {
// FileReader Class:
// Main interface for reading data from files.
// Given a path, the file reader will validate that the path exists,
// and then read all of the data onto a vector.
// This vector can then be used
class FileReader {
  private:
    std::string path;
    std::vector<uint8_t> data;

  public:
    FileReader(const std::string& path);
    ~FileReader();

    std::vector<uint8_t> getData() const;
    bool readFileData();
};

} // namespace Graphics
} // namespace Engine