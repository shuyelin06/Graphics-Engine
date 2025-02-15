#pragma once

#include <vector>

#include "AABB.h"
#include "GJKSupport.h"
#include "math/Transform.h"
#include "math/Vector3.h"

namespace Engine {
using namespace Math;

namespace Physics {
class PhysicsObject;

// CollisionHull Struct:
// Represents a collision hull. Collision objects will store pointers to
// collision hulls
typedef std::vector<Vector3> CollisionHull;

// CollisionObject Class:
// Stores the information for an object that can collide with other
// objects in the physics system.
// Internally, stores a point-set representing a convex collision hull,
// and an AABB of this collision hull.
class CollisionObject : public GJKSupportFunc {
    friend class PhysicsSystem;

    PhysicsObject* phys_object;

    // Point-sets representing the hull of the collider.
    // These points will not change on initialization.
    const CollisionHull* collision_hull;
    const Transform* transform;

    // An AABB of the collision hull, after being transformed.
    AABB broadphase_aabb;

  private:
    CollisionObject(PhysicsObject* phys_obj, const Transform* transform,
                    const CollisionHull* hull);

  public:
    ~CollisionObject();

    // Physics / Collision Methods: Used in the physics engine.
    // Center, FurthestPoint: Lets us query the collision object to see if it's
    // collision hull
    //       collides with another.
    // UpdateBroadphaseAABB: Updates the AABB for use in the AABB tree.
    const Vector3 center(void) const;
    const Vector3 furthestPoint(const Vector3& direction) const;

    void updateBroadphaseAABB(void);

#if (_DEBUG)
    void debugDrawCollider(void);
#endif
};

} // namespace Physics
} // namespace Engine