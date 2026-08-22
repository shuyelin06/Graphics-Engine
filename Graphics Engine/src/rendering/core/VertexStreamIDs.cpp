#include "VertexStreamIDs.h"

#include <assert.h>

namespace Engine
{
namespace Graphics
{
VertexLayout::VertexLayout() { layout_pin = 0; }
VertexLayout::VertexLayout(const VertexLayout& layout)
{
    layout_pin = layout.layout_pin;
}

void VertexLayout::setAllStreams()
{
    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++)
    {
        layout_pin |= 1 << i;
    }
}
void VertexLayout::addVertexStream(VertexDataStream stream)
{
    layout_pin |= (1 << stream);
}
bool VertexLayout::hasVertexStream(VertexDataStream stream) const
{
    return (layout_pin & (1 << stream)) == (1 << stream);
}
bool VertexLayout::vertexLayoutSupports(const VertexLayout& layout) const
{
    return (layout_pin & layout.layout_pin) == layout.layout_pin;
}
size_t VertexLayout::totalStrideSize() const
{
    size_t byte_size = 0;
    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++)
    {
        if (VertexDataStream(i))
        {
            byte_size += VertexStreamStride((VertexDataStream)i);
        }
    }
    return byte_size;
}

bool VertexLayout::operator==(const VertexLayout& layout) const
{
    return layout_pin == layout.layout_pin;
}

size_t VertexLayout::VertexStreamStride(VertexDataStream stream)
{
    return sizeof(float) * 4;
};

} // namespace Graphics
} // namespace Engine