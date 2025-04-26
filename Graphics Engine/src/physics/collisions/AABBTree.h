#pragma once

#include <vector>

#include "CollisionAABB.h"

#if defined(_DEBUG) && defined(DRAW_AABB_EXTENTS)
#define DRAW_AABB_TREE
#endif

namespace Engine {
namespace Physics {
// AABBTree Class:
// Stores a hierarchy of AABB bounding volumes, which can speed up performance
// of the collision system. It can not only be used in as a collision
// broadphase, but can also be used for raycasts.
// https://allenchou.net/2014/02/game-physics-broadphase-dynamic-aabb-tree/
struct AABBNode;

// ColliderPair:
// Contains two AABBs that the AABBTree has found to be colliding
// during its broadphase.
// Can be used by the physics engine for a more precise collision
// check and resolution.
struct ColliderPair {
    CollisionAABB* aabb_1;
    CollisionAABB* aabb_2;

    ColliderPair(CollisionAABB* aabb1, CollisionAABB* aabb2);
};

class AABBTree {
  private:
    AABBNode* root;

    // Represents a margin around AABBs that we'll use in the tree.
    // Nodes in the tree will store the AABB collider + margin in all
    // directions. When AABBs leave this margin, we update them.
    float margin;

    // Stores our collider pairs.
    // Call computeColliderPairs() to populate.
    std::vector<ColliderPair> collider_pairs;

  public:
    AABBTree(float fat_margin);
    ~AABBTree();

    void add(CollisionAABB* aabb);
    void remove(CollisionAABB* aabb);
    void update();

    // Broadphase:
    // Recursively traverses the tree and finds AABB pairs that intersect.
    // Using our tree, we can prune it to avoid some unnecessary computations.
    const std::vector<ColliderPair>& computeColliderPairs();

#if defined(DRAW_AABB_TREE)
    void debugDrawTree() const;
#endif

  private:
    void insertAABB(AABBNode* node, AABBNode** parent);

    void removeAABB(AABBNode* node);
    void correctTreeAfterRemoval(AABBNode* node);

    void findInvalidBeforeUpdate(AABBNode* node, std::vector<CollisionAABB*>& invalid);

    void findColliderPairs(AABBNode* n1, AABBNode* n2);

#if defined(DRAW_AABB_TREE)
    void debugDrawTreeHelper(AABBNode* cur) const;
#endif
};

} // namespace Physics
} // namespace Engine