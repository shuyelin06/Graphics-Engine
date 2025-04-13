#pragma once

#include "BVH.h"
#include "math/Matrix4.h"

namespace Engine {
namespace Datamodel {
struct TLASNode {
    AABB bounds;

    // Left / Right Children (0 if leaf)
    unsigned int left, right;

    // BVH (if leaf; -1 if not a leaf)
    int bvh;

    TLASNode();
};

// Top-Level Acceleration Structure (TLAS):
// A structure that contains multiple BVHs. Less
// efficient in raycasting, but easier to build on the fly.
// Generally, an engine will have many BVHs, which are combined
// (smartly) under TLASes.
class TLAS {
  private:
    std::vector<TLASNode> node_pool;
    std::vector<TransformedBVH> bvh_pool;

  public:
    TLAS();

    // Build the TLAS
    void addTLASNode(BVH* bvh, const Matrix4& transform);
    void build();
    void reset();

    // Get Root
    const TLASNode& getRoot();

    // Raycast into the TLAS. This will
    // traverse the TLAS nodes, and only raycast
    // into intersected BVHs.
    BVHRayCast raycast(const Vector3& origin, const Vector3& direction) const;

  private:
    // Recurse through the BVH to check for an intersection
    void raycastHelper(BVHRay* ray, BVHRayCast* output, UINT node_index) const;
};

} // namespace Datamodel
} // namespace Engine