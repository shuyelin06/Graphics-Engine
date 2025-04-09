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

// Build:
// Given an array of triangles, builds the BVH.
void BVH::build(const std::vector<Triangle>& triangles) {
    // Reset all fields
    node_pool.clear();
    triangle_pool.clear();
    triangle_indices.clear();

    // First, load our array of triangles into the BVH's triangle pool,
    // and add their index to triangle_indices.
    // We use triangle_indices as we'll need to swap around triangles
    // later (and only swapping their indices is more efficient)
    for (const Triangle& tri_data : triangles) {
        BVHTriangle triangle;
        triangle.triangle = tri_data;
        triangle.center = tri_data.center();
        triangle.metadata = nullptr; // TODO: UNUSED

        triangle_indices.push_back(triangle_pool.size());
        triangle_pool.push_back(triangle);
    }

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

    for (int axis = 0; axis < 3; axis++) {
        for (int i = 0; i < node.tri_count; i++) {
            const BVHTriangle& triangle =
                triangle_pool[triangle_indices[node.tri_first + i]];

            const float cost =
                computeSAHCost(index, axis, triangle.center[axis]);
            if (cost < best_cost) {
                best_axis = axis;
                best_pos = triangle.center[axis];
                best_cost = cost;
            }
        }
    }

    // Abort split if we have no split that yields a good cost
    const float parent_cost = node.bounds.area() * node.tri_count;
    if (best_cost >= parent_cost)
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
BVHRayCast BVH::raycast(const Vector3& origin, const Vector3& direction) {
#if defined(DEBUG_BVH_INTERSECTION)
    // Use boolean flags to mark what has and hasn't been intersected for
    // debugging
    for (BVHNode& node : node_pool)
        node.intersected = false;
    for (BVHTriangle& tri : triangle_pool)
        tri.intersected = false;
#endif

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
        ray_cast.hit_triangle = i_hit_triangle;
        ray_cast.t = ray.t;
    }

    return ray_cast;
}

int BVH::raycastHelper(BVHRay& ray, UINT node_index) {
    BVHNode& node = node_pool[node_index];

    // If the ray doesn't intersect the AABB, we can prune this node and
    // all children of the node
    if (!intersectRayWithAABB(ray, node.bounds))
        return -1;

#if defined(DEBUG_BVH_INTERSECTION)
    node.intersected = true;
#endif

    // Otherwise, if the node has children, recurse through the children.
    // If the node doesn't have children, raycast with its triangles.
    int result = -1;

    if (node.isLeaf()) {
        for (int i = 0; i < node.tri_count; i++) {
            const BVHTriangle& triangle =
                triangle_pool[triangle_indices[node.tri_first + i]];

            if (intersectRayWithTriangle(ray, triangle)) {
#if defined(DEBUG_BVH_INTERSECTION)
                triangle_pool[triangle_indices[node.tri_first + i]]
                    .intersected = true;
#endif
                result = i;
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
bool BVH::intersectRayWithTriangle(BVHRay& ray, const BVHTriangle& triangle) {
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
bool BVH::intersectRayWithAABB(const BVHRay& ray, const AABB& aabb) {
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
    for (const BVHNode& node : node_pool) {
        if (!node.isLeaf())
            continue;

        const Color color = node.intersected ? intersect_color : node_color;
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
} // namespace Datamodel
} // namespace Engine