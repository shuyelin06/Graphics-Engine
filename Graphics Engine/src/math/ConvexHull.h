#pragma once

#include <vector>

#include "Transform.h"
#include "Vector3.h"
#include "Triangle.h"
#include "Color.h"

#if defined(_DEBUG)
#define DRAW_CONVEX_HULL
#endif

namespace Engine
{
namespace Math
{
typedef unsigned int UINT;

// ConvexHull Class:
// Defines a 3D Convex Hull in space as a set of triangles and indices.
// Provides an implementation of QuickHull that can be used to generate a 3D
// Convex Hull on any arbitrary point cloud. 
class ConvexHull {
private:
    friend class QuickHullSolver;

    std::vector<Vector3> vertices;
    std::vector<UINT> indices;

    ConvexHull();
  public:
    ~ConvexHull();

    const std::vector<Vector3>& getVertexBuffer() const;
    const std::vector<UINT>& getIndexBuffer() const;

    // Transform all points in the hull by the specified transform
    void transformPoints(const Transform* transform);

#if defined(DRAW_CONVEX_HULL)
    void debugDrawConvexHull() const;
    void debugDrawConvexHull(const Color& color) const;
    void debugDrawConvexHull(const Transform* transform, const Color& color) const;
#endif
};

}
}