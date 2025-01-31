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
// QuickHull Class:
// Uses a naive QuickHull algorithm to generate a 3D convex hull on a set of points.
class ConvexHull {
private:
    std::vector<Triangle> convex_hull;

public:
    ConvexHull();
    ~ConvexHull();

    const std::vector<Triangle>& getConvexHull();

    // Convex Hull generation with the Quick Hull algorithm
    void generateQuickHull(const std::vector<Vector3>& point_cloud);

#if defined(DRAW_CONVEX_HULL)
    void debugDrawConvexHull() const;
    void debugDrawPoints() const;
#endif
};

}
}