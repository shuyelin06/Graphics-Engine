#include "BVH.h"

#include <algorithm>
#include <math.h>

#if defined(DEBUG_BVH)
#include "rendering/VisualDebug.h"
#endif

namespace Engine {
namespace Datamodel {
bool BVHNode::isLeaf() const { return tri_count > 0; }

BVH::BVH() = default;
BVH::~BVH() = default;

const BVHNode& BVH::getBVHRoot() { return node_pool[0]; }
int BVH::size() const { return node_pool.size(); }

// Build:
// Given an array of triangles, builds the BVH.
void BVH::addBVHTriangle(const Triangle& tri_data, void* metadata) {
    BVHTriangle triangle;
    triangle.triangle = tri_data;
    triangle.center = tri_data.center();
    triangle.metadata = metadata; // TODO: UNUSED

    triangle_indices.push_back(triangle_pool.size());
    triangle_pool.push_back(triangle);
}

void BVH::build() {
    if (triangle_pool.size() > 0) {

        // Now, create a root node for our BVH. This node will contain all
        // of our triangles.
        const UINT root_index = allocateNode();
        BVHNode& root = node_pool[root_index];
        root.left = root.right = 0;
        root.tri_first = 0;
        root.tri_count = triangle_pool.size();
        updateBVHNodeAABB(root_index);

        // Recursively subdivide our BVH using the
        // Surface Area Heuristic (SAH)
        subdivide(root_index);
    }
}

// Reset:
// Resets a BVH completely
void BVH::reset() {
    // Reset all fields
    node_pool.clear();
    triangle_pool.clear();
    triangle_indices.clear();
}

// Subdivide:
// Recursively subdivides the BVH
void BVH::subdivide(UINT index) {
    const BVHNode node = node_pool[index];

    // Determine my splitting plane. We do this by iterating through all
    // 3 axes and all centroids of the triangles (contained by the node),
    // and choosing the centroid split that minimizes the resulting AABBs.
    int best_axis = -1;
    float best_pos = 0;
    float best_cost = FLT_MAX;

    std::vector<float> left_cost;
    std::vector<float> right_cost;
    std::vector<float> positions;

    constexpr int NUM_SAMPLES = 3; // MODIFY TO INCREASE RESOLUTION
    constexpr int NUM_DIVISIONS = NUM_SAMPLES + 2;

    for (int axis = 0; axis < 3; axis++) {
        left_cost.clear();
        right_cost.clear();
        positions.clear();

        // Generate my positions
        const float minimum = node.bounds.getMin()[axis];
        const float maximum = node.bounds.getMax()[axis];

        for (int i = 1; i < NUM_DIVISIONS - 1; i++) {
            const float pos =
                i * (maximum - minimum) / (NUM_DIVISIONS - 1) + minimum;
            positions.push_back(pos);
        }

        // Sort our triangles by centroid.
        const std::vector<BVHTriangle>& triangles = triangle_pool;
        std::sort(triangle_indices.begin() + node.tri_first,
                  triangle_indices.begin() + node.tri_first + node.tri_count,
                  [triangles, axis](UINT i0, UINT i1) {
                      return triangles[i0].center[axis] <
                             triangles[i1].center[axis];
                  });

        // Now, iterate through my available positions. For each, grow our AABBs
        // and record the resulting costs.
        const int first = node.tri_first;
        const int last = node.tri_first + node.tri_count - 1;

        AABB left_aabb = AABB();
        int left = node.tri_first;
        int left_count = 0;

        AABB right_aabb = AABB();
        int right = node.tri_first + node.tri_count - 1;
        int right_count = 0;

        for (int i = 0; i < positions.size(); i++) {
            // Left side cost
            const float left_pos = positions[i];

            while (left <= last &&
                   triangles[triangle_indices[left]].center[axis] < left_pos) {
                const Triangle& triangle =
                    triangles[triangle_indices[left]].triangle;
                left_aabb.expandToContain(triangle.vertex(0));
                left_aabb.expandToContain(triangle.vertex(1));
                left_aabb.expandToContain(triangle.vertex(2));
                left++;
                left_count++;
            }

            if (left_count == 0)
                left_cost.push_back(FLT_MAX / 3);
            else
                left_cost.push_back(left_aabb.area() * left_count);

            // Right side cost
            const float right_pos = positions[positions.size() - 1 - i];

            while (right >= first &&
                   triangles[triangle_indices[right]].center[axis] >=
                       right_pos) {
                const Triangle& triangle =
                    triangles[triangle_indices[right]].triangle;
                right_aabb.expandToContain(triangle.vertex(0));
                right_aabb.expandToContain(triangle.vertex(1));
                right_aabb.expandToContain(triangle.vertex(2));
                right--;
                right_count++;
            }

            if (right_count == 0)
                right_cost.push_back(FLT_MAX / 3);
            else
                right_cost.push_back(right_aabb.area() * right_count);
        }

        // Iterate through and choose the minimum cost
        for (int i = 0; i < positions.size(); i++) {
            const float pos = positions[i];
            const float cost =
                left_cost[i] + right_cost[positions.size() - 1 - i];

            if (cost < best_cost) {
                best_axis = axis;
                best_pos = pos;
                best_cost = cost;
            }
        }
    }

    // Abort split if we have no split that yields a good cost.
    // The cost MUST be a proportion of the parent cost or smaller for it
    // to be worth splitting
    const float parent_cost = node.bounds.area() * node.tri_count;
    constexpr float MINIMUM_COST_REDUCTION = 0.50f;
    if (best_cost >= parent_cost * (1 - MINIMUM_COST_REDUCTION))
        return;

    // Iterate through my node's primitives, and swap them so that
    // all primitives on the left size are under the plane, and
    // the rest are above the plane.
    int i = node.tri_first;
    int j = i + node.tri_count - 1;

    while (i <= j) {
        // Case 1:
        // Triangle is under plane. It should be in the left side
        if (triangle_pool[triangle_indices[i]].center[best_axis] < best_pos)
            i++;
        // Case 2:
        // Triangle is on or above plane. It should be on the right side
        else {
            const UINT temp = triangle_indices[i];
            triangle_indices[i] = triangle_indices[j];
            triangle_indices[j] = temp;

            j--;
        }
    }

    // Create two children nodes with these primitives, and recursively
    // subdivide them too
    const UINT tri_start = node.tri_first;
    const UINT tri_count = node.tri_count;

    const UINT left_index = allocateNode();
    node_pool[index].left = left_index;
    node_pool[left_index].tri_first = tri_start;
    node_pool[left_index].tri_count = i - tri_start;
    updateBVHNodeAABB(left_index);

    const UINT right_index = allocateNode();
    node_pool[index].right = right_index;
    node_pool[right_index].tri_first = i;
    node_pool[right_index].tri_count = tri_count - (i - tri_start);
    updateBVHNodeAABB(right_index);

    // Our original node no longer owns any of these triangles. Mark it as so.
    node_pool[index].tri_count = 0;

    subdivide(left_index);
    subdivide(right_index);
}

// AllocateNode:
// Creates a new BVHNode in the node_pool and returns it
// it to be used.
UINT BVH::allocateNode() {
    const UINT index = node_pool.size();
    node_pool.push_back(BVHNode());
    return index;
}

// UpdateBVHNodeAABB:
// Update the AABB of a node by iterating through it's triangles
// and expanding the AABB.
void BVH::updateBVHNodeAABB(UINT index) {
    BVHNode& node = node_pool[index];

    // Reset AABB
    node.bounds = AABB();

    // Iterate through every triangle and expand the AABB
    for (int i = 0; i < node.tri_count; i++) {
        const BVHTriangle& triangle =
            triangle_pool[triangle_indices[node.tri_first + i]];
        node.bounds.expandToContain(triangle.triangle.vertex(0));
        node.bounds.expandToContain(triangle.triangle.vertex(1));
        node.bounds.expandToContain(triangle.triangle.vertex(2));
    }
}

// ComputeSAHCost:
// Computes the SAH cost given some splitting axis
float BVH::computeSAHCost(UINT node, int axis, float pos) {
    AABB left = AABB(), right = AABB();
    int left_count = 0, right_count = 0;

    const BVHNode& bvh_node = node_pool[node];
    for (int i = 0; i < bvh_node.tri_count; i++) {
        const BVHTriangle& tri =
            triangle_pool[triangle_indices[bvh_node.tri_first + i]];

        if (tri.center[axis] < pos) {
            left.expandToContain(tri.triangle.vertex(0));
            left.expandToContain(tri.triangle.vertex(1));
            left.expandToContain(tri.triangle.vertex(2));
            left_count++;
        } else {
            right.expandToContain(tri.triangle.vertex(0));
            right.expandToContain(tri.triangle.vertex(1));
            right.expandToContain(tri.triangle.vertex(2));
            right_count++;
        }
    }

    // Compute the cost. This is a product of the area * count for each box
    // summed. The lower the cost, the better.
    if (left_count == 0 || right_count == 0)
        return FLT_MAX;
    else
        return left.area() * left_count + right.area() * right_count;
}

// Raycast:
// Raycast into the BVH to find the first BVHTriangle
// hit
BVHRayCast BVH::raycast(const Vector3& origin, const Vector3& direction) const {
    BVHRay ray;
    ray.origin = origin;
    ray.direction = direction.unit();
    ray.t = FLT_MAX;

    int i_hit_triangle = raycastHelper(ray, 0);

    BVHRayCast ray_cast;
    if (i_hit_triangle == -1)
        ray_cast.hit = false;
    else {
        ray_cast.hit = true;
        ray_cast.hit_triangle = &triangle_pool[i_hit_triangle];
        ray_cast.t = ray.t;
    }

    return ray_cast;
}

int BVH::raycastHelper(BVHRay& ray, UINT node_index) const {
    const BVHNode& node = node_pool[node_index];

    // If the ray doesn't intersect the AABB, we can prune this node and
    // all children of the node
    if (!IntersectRayWithAABB(ray, node.bounds))
        return -1;

    // Otherwise, if the node has children, recurse through the children.
    // If the node doesn't have children, raycast with its triangles.
    int result = -1;

    if (node.isLeaf()) {
        for (int i = 0; i < node.tri_count; i++) {
            const BVHTriangle& triangle =
                triangle_pool[triangle_indices[node.tri_first + i]];

            if (IntersectRayWithTriangle(ray, triangle)) {
                result = triangle_indices[node.tri_first + i];
            }
        }
    } else {
        int result1 = raycastHelper(ray, node.left);
        int result2 = raycastHelper(ray, node.right);

        if (result1 != -1)
            result = result1;
        if (result2 != -1)
            result = result2;
    }

    return result;
}

// IntersectRayWithTriangle:
// Intersects a ray with a triangle. Returns true
// if the ray intersects the triangle, and updates the ray's
// "t" parameter (storing the distance)
// Implements the Möller–Trumbore Ray-Triangle Intersection Algorithm
bool BVH::IntersectRayWithTriangle(BVHRay& ray, const BVHTriangle& triangle) {
    constexpr float EPSILON = 0.0001f;

    const Triangle& tri = triangle.triangle;

    // Find our triangle's edges. These two edges
    // form a plane that the triangle is on.
    const Vector3 edge1 = tri.vertex(1) - tri.vertex(0);
    const Vector3 edge2 = tri.vertex(2) - tri.vertex(0);

    // First, check if our ray is parallel to the triangle.
    // We do this by finding the normal of the plane formed
    // by the ray / edge2 of the triangle, and seeing if
    // it is orthogonal to edge1 of the triangle
    const Vector3 h = ray.direction.cross(edge2);
    const float a = edge1.dot(h);

    if (-EPSILON < a && a < EPSILON)
        return false; // Parallel

    // Now, solve a system of equations using Cramer's rule
    // to find the intersection point.
    const float f = 1 / a;
    const Vector3 s = ray.origin - tri.vertex(0);
    const float u = f * s.dot(h);

    if (u < 0 || u > 1)
        return false;

    const Vector3 q = s.cross(edge1);
    const float v = f * ray.direction.dot(q);
    if (v < 0 || u + v > 1)
        return false;

    const float t = f * edge2.dot(q);
    if (t > EPSILON && t < ray.t) {
        // We found an intersection
        ray.t = t;
        return true;
    }

    return false;
}

// IntersectRayWithAABB:
// Intersects the ray with an AABB. Returns true on intersection, false
// otherwise.
// Performs this without branches.
bool BVH::IntersectRayWithAABB(const BVHRay& ray, const AABB& aabb) {
    const Vector3& origin = ray.origin;
    const Vector3& direction = ray.direction;

    const Vector3& minimum = aabb.getMin();
    const Vector3& maximum = aabb.getMax();

    const float tx1 = (minimum.x - origin.x) / direction.x;
    const float tx2 = (maximum.x - origin.x) / direction.x;
    float tmin = std::min(tx1, tx2);
    float tmax = std::max(tx1, tx2);

    const float ty1 = (minimum.y - origin.y) / direction.y;
    const float ty2 = (maximum.y - origin.y) / direction.y;
    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));

    const float tz1 = (minimum.z - origin.z) / direction.z;
    const float tz2 = (maximum.z - origin.z) / direction.z;
    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));

    return tmax >= tmin && tmin < ray.t && tmax > 0;
}

#if defined(DEBUG_BVH)
void BVH::debugDrawBVH() const {
    const Color intersect_color = Color::Green();

    const Color triangle_color = Color::Blue();
    for (const BVHTriangle& tri : triangle_pool) {
        const Color color = tri.intersected ? intersect_color : triangle_color;

        Graphics::VisualDebug::DrawLine(tri.triangle.vertex(0),
                                        tri.triangle.vertex(1), color);
        Graphics::VisualDebug::DrawLine(tri.triangle.vertex(1),
                                        tri.triangle.vertex(2), color);
        Graphics::VisualDebug::DrawLine(tri.triangle.vertex(2),
                                        tri.triangle.vertex(0), color);

        Graphics::VisualDebug::DrawLine(
            tri.center, tri.center + tri.triangle.normal() * 2.5f,
            Color::White());
    }

    const Color node_color = Color::White();
    Vector3 corners[8];
    for (const BVHNode& node : node_pool) {
        if (!node.isLeaf())
            continue;

        const Color color = node.intersected ? intersect_color : node_color;
        node.bounds.fillArrWithPoints(corners);

        const Vector3& minimum = node.bounds.getMin();
        const Vector3& maximum = node.bounds.getMax();

        Graphics::VisualDebug::DrawLine(
            Vector3(minimum.x, minimum.y, minimum.z),
            Vector3(maximum.x, minimum.y, minimum.z), color);
        Graphics::VisualDebug::DrawLine(
            Vector3(maximum.x, minimum.y, minimum.z),
            Vector3(maximum.x, maximum.y, minimum.z), color);
        Graphics::VisualDebug::DrawLine(
            Vector3(maximum.x, maximum.y, minimum.z),
            Vector3(minimum.x, maximum.y, minimum.z), color);
        Graphics::VisualDebug::DrawLine(
            Vector3(minimum.x, maximum.y, minimum.z),
            Vector3(minimum.x, minimum.y, minimum.z), color);

        Graphics::VisualDebug::DrawLine(
            Vector3(minimum.x, minimum.y, maximum.z),
            Vector3(maximum.x, minimum.y, maximum.z), color);
        Graphics::VisualDebug::DrawLine(
            Vector3(maximum.x, minimum.y, maximum.z),
            Vector3(maximum.x, maximum.y, maximum.z), color);
        Graphics::VisualDebug::DrawLine(
            Vector3(maximum.x, maximum.y, maximum.z),
            Vector3(minimum.x, maximum.y, maximum.z), color);
        Graphics::VisualDebug::DrawLine(
            Vector3(minimum.x, maximum.y, maximum.z),
            Vector3(minimum.x, minimum.y, maximum.z), color);

        Graphics::VisualDebug::DrawLine(
            Vector3(minimum.x, minimum.y, minimum.z),
            Vector3(minimum.x, minimum.y, maximum.z), color);
        Graphics::VisualDebug::DrawLine(
            Vector3(minimum.x, maximum.y, minimum.z),
            Vector3(minimum.x, maximum.y, maximum.z), color);
        Graphics::VisualDebug::DrawLine(
            Vector3(maximum.x, minimum.y, minimum.z),
            Vector3(maximum.x, minimum.y, maximum.z), color);
        Graphics::VisualDebug::DrawLine(
            Vector3(maximum.x, maximum.y, minimum.z),
            Vector3(maximum.x, maximum.y, maximum.z), color);
    }
}
#endif

// --- Transformed BVH ---
TransformedBVH::TransformedBVH(BVH* _bvh, const Matrix4& m_transform) {
    bvh = _bvh;
    m_inverse = m_transform.inverse();

    bounds = AABB();
    Vector3 aabb_bounds[8];
    bvh->getBVHRoot().bounds.fillArrWithPoints(aabb_bounds);

    for (int i = 0; i < 8; i++) {
        // Transform my point into world space, and expand our TLAS node AABB
        // with it
        const Vector3 transformed =
            (m_transform * Vector4(aabb_bounds[i], 1.f)).xyz();
        bounds.expandToContain(transformed);
    }
}

const AABB& TransformedBVH::getBounds() const { return bounds; }
BVHRayCast TransformedBVH::raycast(const Vector3& origin,
                                   const Vector3& direction) const {
    // Transform my origin and direction. For the origin, we apply
    // all transforms (scale, translate, rotate). For the direction,
    // we only apply rotate + scale.
    const Vector3 local_origin = (m_inverse * Vector4(origin, 1.f)).xyz();
    const Vector3 local_direction = (m_inverse * Vector4(direction, 0.f)).xyz();

    const BVHRayCast raycast = bvh->raycast(local_origin, local_direction);
    return raycast;
}

} // namespace Datamodel
} // namespace Engine