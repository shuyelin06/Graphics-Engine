#pragma once

#include <string>

namespace Engine {
enum FileExtension { UNKNOWN, GLTF, GLB, PNG };

// Container for resource files, located in "data/"
class DataFilePath {
  private:
    std::string path;
    FileExtension extension;

  public:
    DataFilePath(const std::string& path);

    FileExtension getExtension() const;
    const std::string& getPath() const;
    const std::string getFullPath() const;

    size_t computeHash() const;
};

} // namespace Engine