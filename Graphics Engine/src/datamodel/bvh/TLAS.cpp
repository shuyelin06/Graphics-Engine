#include "TLAS.h"

#include <assert.h>

namespace Engine {
namespace Datamodel {
TLASNode::TLASNode() {
    bounds = AABB();
    left = right = 0;
    bvh = -1;
}

TLAS::TLAS() = default;

// --- TLAS Building ---
// AddTLASNode:
// Adds a node to the TLAS, which is a transformed BVH.
void TLAS::addTLASNode(BVH* bvh, const Matrix4& transform) {
    // Add BVH to pool only if it has nodes
    if (bvh->size() > 0) {
        // Add my BVH to the pool
        int bvh_index = bvh_pool.size();
        bvh_pool.push_back(TransformedBVH(bvh, transform));

        // Create a node for this BVH
        TLASNode node = TLASNode();
        node.bounds = bvh_pool[bvh_index].getBounds();
        node.bvh = bvh_index;
        node_pool.push_back(node);
    }
}

// Build:
// Iteratively traverses the TLAS nodes and attempts to combine the nodes
// whose union MINIMIZES the area of the resultant AABB.
void TLAS::build() {
    // First, add all of our nodes by index to a list. This list
    // will track all nodes that do not have a parent.
    std::vector<unsigned int> unassigned;
    for (int i = 0; i < node_pool.size(); i++) {
        unassigned.push_back(i);
    }

    // While we have >1 unassigned nodes, merge the last unassigned node
    // with the one minimizing their total surface area.
    while (unassigned.size() > 1) {
        const unsigned int A = unassigned[unassigned.size() - 1];
        unassigned.pop_back();

        // Select the node whose AABB union with A minimizes the surface
        // area
        int index = 0;
        unsigned int B = unassigned[index];
        float min_SA =
            node_pool[A].bounds.unionWith(node_pool[B].bounds).area();

        for (int i = 1; i < unassigned.size(); i++) {
            const float SA =
                node_pool[A]
                    .bounds.unionWith(node_pool[unassigned[i]].bounds)
                    .area();

            if (SA < min_SA) {
                index = i;
                B = unassigned[i];
                min_SA = SA;
            }
        }

        unassigned.erase(unassigned.begin() + index);

        // Now, create a new TLAS node and add it to our TLAS.
        TLASNode new_node;
        new_node.bounds = node_pool[A].bounds.unionWith(node_pool[B].bounds);
        new_node.left = A;
        new_node.right = B;
        new_node.bvh = -1;

        const int new_node_index = node_pool.size();
        node_pool.push_back(new_node);
        unassigned.push_back(new_node_index);
    }
}

// BuildFast:
// Fast build that just merges every adjacent node in the list.
// Use when adjacent nodes have some sort of spatial relation to each other.
// One good use of this is terrain chunks; we naturally add chunks to the TLAS
// in an order where adjacent chunks are also adjacent in the list,
// so we can exploit this for faster build times.
void TLAS::buildFast() {
    std::vector<unsigned int> unassigned_temp;

    // First, add all of our nodes by index to a list. This list
    // will track all nodes that do not have a parent.
    std::vector<unsigned int> unassigned;
    for (int i = 0; i < node_pool.size(); i++) {
        unassigned.push_back(i);
    }

    // While we have >1 unassigned nodes, merge the last unassigned node
    // with the one minimizing their total surface area.
    while (unassigned.size() > 1) {
        unassigned_temp.clear();

        while (unassigned.size() >= 2) {
            const unsigned int A = unassigned.back();
            unassigned.pop_back();

            const unsigned int B = unassigned.back();
            unassigned.pop_back();

            // Now, create a new TLAS node and add it to our TLAS.
            TLASNode new_node;
            new_node.bounds =
                node_pool[A].bounds.unionWith(node_pool[B].bounds);
            new_node.left = A;
            new_node.right = B;
            new_node.bvh = -1;

            const int new_node_index = node_pool.size();
            node_pool.push_back(new_node);
            unassigned_temp.push_back(new_node_index);
        }

        if (unassigned.size() == 1)
            unassigned_temp.push_back(unassigned.back());

        unassigned = unassigned_temp;
    }
}

// Reset:
// Clears the TLAS fields
void TLAS::reset() {
    node_pool.clear();
    bvh_pool.clear();
}

// Root is the last node of the TLASNode vector.
const TLASNode& TLAS::getRoot() { return node_pool[node_pool.size() - 1]; }

// Raycast into the TLAS. This will
// traverse the TLAS nodes, and only raycast
// into intersected BVHs.
BVHRayCast TLAS::raycast(const Vector3& origin,
                         const Vector3& direction) const {
    BVHRay ray;
    ray.origin = origin;
    ray.direction = direction.unit();
    ray.t = FLT_MAX;

    BVHRayCast output;
    output.hit = false;

    if (node_pool.size() > 0)
        raycastHelper(&ray, &output, node_pool.size() - 1);

    return output;
}

void TLAS::raycastHelper(BVHRay* ray, BVHRayCast* output,
                         UINT node_index) const {
    const TLASNode& node = node_pool[node_index];

    // If we don't intersect the node's AABB, we guaranteed will not
    // intersect any of the node's children as well.
    if (!BVH::IntersectRayWithAABB(*ray, node.bounds))
        return;

    // If the node is a leaf, we raycast with the BVH. Otherwise,
    // we recurse to the node's children IF AND ONLY IF
    // the ray intersects the node's AABB.
    if (node.bvh != -1) {
        // The TLASNode is a leaf. Raycast into the BVH.
        const BVHRayCast result =
            bvh_pool[node.bvh].raycast(ray->origin, ray->direction);

        // If the result is closer along the ray than what we have right now,
        // take it.
        if (result.hit && result.t < ray->t) {
            *output = result;
            ray->t = result.t;
        }
    } else {
        // The TLASNode is a branch. Traverse into the children ONLY IF
        // the ray intersects the AABB.
        raycastHelper(ray, output, node.left);
        raycastHelper(ray, output, node.right);
    }
}
} // namespace Datamodel
} // namespace Engine