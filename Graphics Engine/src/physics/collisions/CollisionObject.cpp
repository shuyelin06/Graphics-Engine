#include "CollisionObject.h"

#if defined(_DEBUG)
#include "math/QuickHull.h"
#endif

namespace Engine {
namespace Physics {
CollisionObject::CollisionObject(PhysicsObject* phys_obj,
                                 const Transform* _transform,
                                 const CollisionHull* hull)
    : collision_hull(hull), transform(_transform) {
    phys_object = phys_obj;

    broadphase_aabb = CollisionAABB();
    broadphase_aabb.collider = this;
}
CollisionObject::~CollisionObject() = default;

// GJKSupport Methods:
// Lets us query the CollisionObject as a support function, so that it can be
// used in the GJK algorithm for collision detection.
const Vector3 CollisionObject::center(void) const {
    Vector3 center = Vector3(0, 0, 0);

    for (const Vector3& point : *collision_hull)
        center += point;
    center /= collision_hull->size();

    center += transform->getPosition();

    return center;
}

const Vector3 CollisionObject::furthestPoint(const Vector3& direction) const {
    if (collision_hull->size() == 0) {
        return Vector3(0, 0, 0);
    }

    const Vector3 direc = direction.unit();
    const Matrix4 m_transform = transform->transformMatrix();

    Vector3 furthest =
        (m_transform * Vector4((*collision_hull)[0], 1.0f)).xyz();

    for (const Vector3& point : *collision_hull) {
        const Vector3 transformed = (m_transform * Vector4(point, 1.0f)).xyz();
        if (transformed.dot(direc) >= furthest.dot(direc)) {
            furthest = transformed;
        }
    }

    return furthest;
}

// UpdateBroadphaseAABB:
// Updates the AABB extents to encompass the translated convex hull,
// so that it can be used in the broadphase collision pass.
void CollisionObject::updateBroadphaseAABB(void) {
    broadphase_aabb = CollisionAABB();

    const Matrix4 m_transform = transform->transformMatrix();

    for (const Vector3& point : *collision_hull) {
        const Vector3 transformed = (m_transform * Vector4(point, 1.0f)).xyz();
        broadphase_aabb.expandToContain(transformed);
    }
}

#if (_DEBUG)
void CollisionObject::debugDrawCollider(void) {
    QuickHullSolver solver;
    solver.computeConvexHull(*collision_hull);
    ConvexHull* hull = solver.getHull();
    hull->transformPoints(transform);
    hull->debugDrawConvexHull();
    delete hull;
}
#endif

} // namespace Physics
} // namespace Engine