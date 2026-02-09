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

VertexLayout::VertexLayout() { layout_pin = 0; }
VertexLayout::VertexLayout(const VertexLayout& layout) {
    layout_pin = layout.layout_pin;
}

void VertexLayout::setAllStreams() {
    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++) {
        if (hasVertexStream((VertexDataStream)i)) {
            layout_pin |= 1 << i;
        }
    }
}
void VertexLayout::addVertexStream(VertexDataStream stream) {
    layout_pin |= stream;
}
bool VertexLayout::hasVertexStream(VertexDataStream stream) const {
    return (layout_pin & stream) == stream;
}
bool VertexLayout::vertexLayoutSupports(const VertexLayout& layout) const {
    return (layout_pin & layout.layout_pin) == layout.layout_pin;
}
size_t VertexLayout::totalStrideSize() const {
    size_t byte_size = 0;
    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++) {
        if (VertexDataStream(i)) {
            byte_size += VertexStreamStride((VertexDataStream)i);
        }
    }
    return byte_size;
}

bool VertexLayout::operator==(const VertexLayout& layout) const {
    return layout_pin == layout.layout_pin;
}

size_t VertexLayout::VertexStreamStride(VertexDataStream stream) {
    assert(0 <= stream && stream < BINDABLE_STREAM_COUNT);
    return StreamStrides[stream];
};

} // namespace Graphics
} // namespace Engine