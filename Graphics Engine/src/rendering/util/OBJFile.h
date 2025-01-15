#pragma once

#include <string>

namespace Engine
{
namespace Graphics
{
// OBJFile Class:
// Class that provides an interface for working with
// OBJ files. Allows reading to and writing from these files.
// WIP TODO: Move OBJ file parsing to this class
class OBJFile 
{
private:
    std::string path;

public:
    OBJFile(const std::string& path);

};

}}