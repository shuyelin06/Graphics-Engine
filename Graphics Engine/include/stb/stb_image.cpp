// We don't need most of the image decoders, so we'll only define the ones we
// use here to reduce the code footprint
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_JPEG

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"