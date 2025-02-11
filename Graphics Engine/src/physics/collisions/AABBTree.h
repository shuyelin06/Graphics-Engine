#pragma once

#include <vector>

#include "AABB.h"

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

class AABBTree {
  private:
    AABBNode* root;

    // Represents a margin around AABBs that we'll use in the tree.
    // Nodes in the tree will store the AABB collider + margin in all
    // directions. When AABBs leave this margin, we update them.
    float margin;

  public:
    AABBTree(float fat_margin);
    ~AABBTree();

    void add(AABB* aabb);
    void remove(AABB* aabb);
    void update();

#if defined(DRAW_AABB_TREE)
    void debugDrawTree() const;
#endif

  private:
    void insertAABB(AABBNode* node, AABBNode** parent);

    void removeAABB(AABBNode* node);
    void correctTreeAfterRemoval(AABBNode* node);

    void findInvalidBeforeUpdate(AABBNode* node, std::vector<AABB*>& invalid);

#if defined(DRAW_AABB_TREE)
    void debugDrawTreeHelper(AABBNode* cur) const;
#endif
};

} // namespace Physics
} // namespace Engine