#pragma once

#include <vector>

#include "../Vector3.h"
#include "../Triangle.h"

#if defined(_DEBUG)
#define DRAW_CONVEX_HULL
#endif

namespace Engine
{
namespace Math
{
// ConvexHull Class:
// Defines a 3D Convex Hull in space as a set of triangles and indices.
// Provides an implementation of QuickHull that can be used to generate a 3D
// Convex Hull on any arbitrary point cloud. 
class ConvexHull {
private:
    std::vector<Triangle> convex_hull;

public:
    ConvexHull();
    ~ConvexHull();

    const std::vector<Triangle>& getConvexHull();

    // Convex Hull generation with Quick Hull
    static ConvexHull* QuickHull(const std::vector<Vector3>& point_cloud);

#if defined(DRAW_CONVEX_HULL)
    void debugDrawConvexHull() const;
    void debugDrawPoints() const;
#endif
};

}
}