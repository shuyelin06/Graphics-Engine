#include "FileReader.h"

#include <filesystem>
#include <fstream>

namespace Engine {
namespace Graphics {
FileReader::FileReader(const std::string& _path) : path(_path), data(0) {}
FileReader::~FileReader() = default;

std::vector<uint8_t> FileReader::getData() const { return data; }

bool FileReader::readFileData() {
    std::ifstream file_in(path, std::ios::binary);
    
    if (file_in.fail()) {
        return false;
    }

    // Don't skip new lines
    file_in.unsetf(std::ios::skipws);

    // Get file size for vector reservation
    std::streampos file_size;
    file_in.seekg(0, std::ios::end);
    file_size = file_in.tellg();
    file_in.seekg(0, std::ios::beg);

    data.resize(file_size);
    file_in.read(reinterpret_cast<char*>(&data[0]), file_size);

    file_in.close();

    return true;
}

} // namespace Graphics
} // namespace Engine