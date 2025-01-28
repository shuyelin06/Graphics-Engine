#include "AABB.h"

#include <math.h>

#if defined(DRAW_AABB_EXTENTS)
#include "rendering/VisualDebug.h"
#endif

namespace Engine {
namespace Math {
AABB::AABB() : minimum(Vector3::VectorMax()), maximum(Vector3::VectorMin()) {}
AABB::AABB(const Vector3& center) : minimum(center), maximum(center) {}
AABB::~AABB() = default;

// Getters:
// Return properties of the AABB.
float AABB::volume() const {
    const Vector3 difference = maximum - minimum;
    return fabsf(difference.x * difference.y * difference.z);
}
const Vector3& AABB::getMin() const { return minimum; }
const Vector3& AABB::getMax() const { return maximum; }

// ExpandToContain:
// Given a point, expands the AABB so that it includes the point.
void AABB::expandToContain(const Vector3& point) {
    minimum = minimum.componentMin(point);
    maximum = maximum.componentMax(point);
}

#if defined(DRAW_AABB_EXTENTS)
void AABB::debugDrawExtents() const {
    Graphics::VisualDebug::DrawLine(Vector3(minimum.x, minimum.y, minimum.z),
                                    Vector3(maximum.x, minimum.y, minimum.z),
                                    Color::Blue());
    Graphics::VisualDebug::DrawLine(Vector3(maximum.x, minimum.y, minimum.z),
                                    Vector3(maximum.x, maximum.y, minimum.z),
                                    Color::Blue());
    Graphics::VisualDebug::DrawLine(Vector3(maximum.x, maximum.y, minimum.z),
                                    Vector3(minimum.x, maximum.y, minimum.z),
                                    Color::Blue());
    Graphics::VisualDebug::DrawLine(Vector3(minimum.x, maximum.y, minimum.z),
                                    Vector3(minimum.x, minimum.y, minimum.z),
                                    Color::Blue());

    Graphics::VisualDebug::DrawLine(Vector3(minimum.x, minimum.y, maximum.z),
                                    Vector3(maximum.x, minimum.y, maximum.z),
                                    Color::Blue());
    Graphics::VisualDebug::DrawLine(Vector3(maximum.x, minimum.y, maximum.z),
                                    Vector3(maximum.x, maximum.y, maximum.z),
                                    Color::Blue());
    Graphics::VisualDebug::DrawLine(Vector3(maximum.x, maximum.y, maximum.z),
                                    Vector3(minimum.x, maximum.y, maximum.z),
                                    Color::Blue());
    Graphics::VisualDebug::DrawLine(Vector3(minimum.x, maximum.y, maximum.z),
                                    Vector3(minimum.x, minimum.y, maximum.z),
                                    Color::Blue());

    Graphics::VisualDebug::DrawLine(Vector3(minimum.x, minimum.y, minimum.z),
                                    Vector3(minimum.x, minimum.y, maximum.z),
                                    Color::Blue());
    Graphics::VisualDebug::DrawLine(Vector3(minimum.x, maximum.y, minimum.z),
                                    Vector3(minimum.x, maximum.y, maximum.z),
                                    Color::Blue());
    Graphics::VisualDebug::DrawLine(Vector3(maximum.x, minimum.y, minimum.z),
                                    Vector3(maximum.x, minimum.y, maximum.z),
                                    Color::Blue());
    Graphics::VisualDebug::DrawLine(Vector3(maximum.x, maximum.y, minimum.z),
                                    Vector3(maximum.x, maximum.y, maximum.z),
                                    Color::Blue());
}
#endif

} // namespace Math
} // namespace Engine