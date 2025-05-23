#pragma once
#pragma once

#define DEBUG_BVH

// On raycast, flags what nodes and triangles
// the ray has intersected and displays them in green.
// Only displays this for the last raycasta before
// debugDraw() is called.
#define DEBUG_BVH_INTERSECTION

#include <vector>

#include "math/AABB.h"
#include "math/Matrix4.h"
#include "math/Triangle.h"
#include "math/Vector3.h"

#define BVH_NO_INTERSECTION -1

namespace Engine {
using namespace Math;
namespace Datamodel {
typedef unsigned int UINT;

// BVHTriangle:
// Represents a triangle that the BVH uses.
// Stores the base triangle data as well as
// additional metadata.
struct BVHTriangle {
    Triangle triangle;
    Vector3 center;

    void* metadata;

#if defined(DEBUG_BVH_INTERSECTION)
    bool intersected;
#endif
};

// BVHRay:
// Represents a ray that the BVH uses
struct BVHRay {
    Vector3 origin;
    Vector3 direction;

    // Stores the distance we found a ray-triangle
    // intersection at
    float t;
};

// BVHNode:
// A single node in the BVH.
struct BVHNode {
    // (x,y,z) bounds of the node
    AABB bounds;

    // Children, given as indices into
    // a "pool" (array) of BVHNodes
    UINT left;
    UINT right;

    // Triangles, given as a starting index and
    // a count of triangles into the tri_indices array
    UINT tri_first;
    UINT tri_count;

#if defined(DEBUG_BVH_INTERSECTION)
    bool intersected;
#endif

    // Returns true if tri_count > 0, which only
    // happens if the node is a leaf.
    bool isLeaf() const;
};

// RayCast Information
struct BVHRayCast {
    bool hit;

    const BVHTriangle* hit_triangle;
    float t;
};

// Bounding Volume Hierarchy (BVH) Class:
// Represents a bounding volume hierarchy, which is a
// spatial acceleration structure for raycasting.
// https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/
class BVH {
  private:
    std::vector<BVHNode> node_pool;

    std::vector<BVHTriangle> triangle_pool;
    std::vector<UINT> triangle_indices;

  public:
    BVH();
    ~BVH();

    // Build the BVH
    void addBVHTriangle(const Triangle& triangle, void* metadata);
    void build();
    void reset();

    // Raycast into the BVH to find the first BVHTriangle
    // hit. Returns the index of this triangle, or -1 on no hit.
    BVHRayCast raycast(const Vector3& origin, const Vector3& direction) const;

    // Get BVH data
    const BVHNode& getBVHRoot();
    int size() const;

#if defined(DEBUG_BVH)
    void debugDrawBVH() const;
#endif

  private:
    // Node creation / updating
    UINT allocateNode();
    void updateBVHNodeAABB(UINT node);
    // BVH subdividing with SAH
    void subdivide(UINT node);
    // SAH cost evaluator
    float computeSAHCost(UINT node, int axis, float pos);

    // Recurse through the BVH to check for an intersection
    int raycastHelper(BVHRay& ray, UINT node_index) const;

  public:
    // Ray-Triangle Intersection
    static bool IntersectRayWithTriangle(BVHRay& ray,
                                         const BVHTriangle& triangle);
    // Ray-AABB Intersection
    static bool IntersectRayWithAABB(const BVHRay& ray, const AABB& aabb);
};

// We also support transformed BVH's, so we can reuse the same BVH
// (for say, the same mesh) on different transforms.
class TransformedBVH {
  private:
    BVH* bvh;

    Matrix4 m_inverse;
    AABB bounds;

  public:
    TransformedBVH(BVH* bvh, const Matrix4& m_transform);

    // Get world AABB
    const AABB& getBounds() const;

    // Raycast on the transformed BVH. This uses the BVH raycast, but
    // transforms the ray into the BVH's local space.
    BVHRayCast raycast(const Vector3& origin, const Vector3& direction) const;
};

} // namespace Datamodel
} // namespace Engine