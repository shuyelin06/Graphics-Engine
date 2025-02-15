#pragma once

#include <vector>

#include "math/Vector3.h"

#if defined(_DEBUG)
#define DRAW_AABB_EXTENTS
#include "math/Color.h"
#endif

namespace Engine {
using namespace Math;

namespace Physics {
class AABBTree;
struct AABBNode;
class CollisionObject;
class PhysicsSystem;

// AxisAlignedBoundingBox (AABB):
// Represents an AABB in 3D space, given by its lower left corner and upper
// right corner.
class AABB {
friend class AABBTree;
friend struct AABBNode;
friend class CollisionObject;
friend class PhysicsSystem;

  private:
    Vector3 minimum;
    Vector3 maximum;

    // Stores a reference to its node in the AABBTree
    AABBNode* node;
    // Stores a reference to its collider object
    CollisionObject* collider;

  public:
    AABB();
    AABB(const Vector3& center);
    ~AABB();

    float volume() const;

    const Vector3& getMin() const;
    const Vector3& getMax() const;

    bool contains(const AABB& aabb) const;
    bool contains(const Vector3& point) const;
    bool intersects(const AABB& aabb) const;

    AABB unionWith(const AABB& aabb) const;

    // If this is called and the node is not null, the AABBTree should be
    // updated.
    void expandToContain(const std::vector<Vector3>& points);
    void expandToContain(const Vector3& point);

    // Clears the AABB extents
    void reset();

#if defined(DRAW_AABB_EXTENTS)
    void debugDrawExtents() const;
    void debugDrawExtents(const Color& color) const;
#endif
};

} // namespace Physics
} // namespace Engine