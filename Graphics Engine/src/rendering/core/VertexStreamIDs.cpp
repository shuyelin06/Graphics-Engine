#include "VertexStreamIDs.h"

#include <assert.h>

namespace Engine {
namespace Graphics {
static unsigned int StreamStrides[BINDABLE_STREAM_COUNT] = {
    3 * sizeof(float), // POSITION
    2 * sizeof(float), // TEXTURE
    3 * sizeof(float), // NORMAL
    3 * sizeof(float), // COLOR
    4 * sizeof(float), // JOINTS
    4 * sizeof(float)  // WEIGHTS
};

bool LayoutPinHas(uint16_t pin, unsigned int stream) {
    assert(0 <= stream && stream < BINDABLE_STREAM_COUNT);
    return (pin & (1 << stream)) == (1 << stream);
}
unsigned int StreamVertexStride(unsigned int stream) {
    assert(0 <= stream && stream < BINDABLE_STREAM_COUNT);
    return StreamStrides[stream];
}

uint16_t VertexStreamLayoutPin(VertexDataStream* streams, size_t size) {
    uint16_t pin = 0;

    for (int i = 0; i < size; i++)
        pin |= (1 << streams[i]);

    return pin;
}

} // namespace Graphics
} // namespace Engine