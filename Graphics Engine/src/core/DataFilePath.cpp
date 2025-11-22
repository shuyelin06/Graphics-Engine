#include "DataFilePath.h"

#include <regex>
#include <unordered_map>

namespace Engine {
DataFilePath::DataFilePath(const std::string& _path) : path(_path) {
    std::regex name_pattern("(?:.+/)*([a-zA-Z0-9]+)\\.([a-zA-Z]+)");
    std::smatch match;
    regex_search(path, match, name_pattern);

    const std::string extension_str = match[2];
    static const std::unordered_map<std::string, FileExtension>
        extension_map{{"png", PNG}, {"gltf", GLTF}, {"glb", GLB}};
    extension = UNKNOWN;
    if (extension_map.contains(extension_str))
        extension = extension_map.at(extension_str);
}

FileExtension DataFilePath::getExtension() const { return extension; }
const std::string& DataFilePath::getPath() const { return path; }
const std::string DataFilePath::getFullPath() const { return "data/" + path; }

size_t DataFilePath::computeHash() const {
    std::hash<std::string> hash_fn;
    const size_t hash = hash_fn(path);
    return hash;
}

} // namespace Engine