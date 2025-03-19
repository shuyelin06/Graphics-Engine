#include "GLTFFile.h"

// The GLTFFile uses the cgltf library to read GLTF files.
// See https://github.com/jkuhlmann/cgltf
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

namespace Engine {
namespace Graphics {
GLTFFile::GLTFFile(const std::string& _path) : path(_path) {}

bool GLTFFile::readFromFile() {
    cgltf_options options = {};
    cgltf_data* data = NULL;

    // Parse my GLTF file
    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);

    // If success, interpret the content of the parser to read it into
    // data our engine can use
    if (result == cgltf_result_success) {
        // TODO
        int t = 5;

        // Finally, free any used memory
        cgltf_free(data);

        return true;
    } else
        return false;
}

} // namespace Graphics
} // namespace Engine