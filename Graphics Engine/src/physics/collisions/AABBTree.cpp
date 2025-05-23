#include "AABBTree.h"

#include <assert.h>

namespace Engine {
namespace Physics {
ColliderPair::ColliderPair(CollisionAABB* aabb1, CollisionAABB* aabb2) {
    aabb_1 = aabb1;
    aabb_2 = aabb2;
}

// AABBNode Class:
// Represents a node in the tree. A node has a parent, and 2 children.
// If the node is a branch, then it is an AABB that contains other
// AABB colliders. If the node is a leaf, then it is an AABB collider.
struct AABBNode {
    AABBNode* parent;
    AABBNode* children[2];

    // Node AABB that is guaranteed (by property of the AABB tree)
    // to contain the AABB of all its children
    CollisionAABB aabb;

    // Collider AABB. Only leaves store collider AABB data
    CollisionAABB* data;

    AABBNode() {
        parent = nullptr;
        children[0] = nullptr;
        children[1] = nullptr;
        aabb = CollisionAABB();
        data = nullptr;
    }
    ~AABBNode() {}

    // UpdateAABB:
    // Updates the node's AABB. If it is a leaf, the AABB will match the data.
    // Otherwise, the AABB will contain all of the node's children.
    void updateAABB(float margin) {
        if (isLeaf()) {
            const Vector3 margin_vector = Vector3(margin, margin, margin);

            aabb = CollisionAABB();
            aabb.expandToContain(data->getMin() - margin_vector);
            aabb.expandToContain(data->getMax() + margin_vector);
        } else
            aabb = children[0]->aabb.unionWith(children[1]->aabb);
    }

    // IsLeaf:
    // Returns if the node is a leaf or not. If a node is a leaf, both
    // children will be null.
    bool isLeaf() { return children[0] == nullptr; }

    // SefLeaf:
    // Set the node to be a leaf, with data == aabb
    void setLeaf(CollisionAABB* aabb) {
        data = aabb;
        aabb->node = this;

        children[0] = nullptr;
        children[1] = nullptr;
    }

    // GetSibling:
    // If the node is a leaf, returns the other child of the node's parent.
    AABBNode* getSibling() const {
        if (this == parent->children[0])
            return parent->children[1];
        else if (this == parent->children[1])
            return parent->children[0];
        else
            return nullptr;
    }

    // SetBranch:
    // Set the node to be a branch, with children as specified. Data == nullptr.
    void setBranch(AABBNode* left, AABBNode* right) {
        data = nullptr;

        left->parent = this;
        right->parent = this;

        children[0] = left;
        children[1] = right;
    }
};

AABBTree::AABBTree(float fat_margin) : collider_pairs() {
    root = nullptr;
    margin = fat_margin;
}
AABBTree::~AABBTree() {
    if (root != nullptr)
        delete root;
}

// AddAABB:
// Adds an AABB collider into the tree. If the tree is empty, sets the
// collider as the root. Otherwise, recursively goes into the tree
// to find the best place to insert the node.
void AABBTree::add(CollisionAABB* aabb) {
    if (root != nullptr) {
        AABBNode* node = new AABBNode();
        node->setLeaf(aabb);
        node->updateAABB(margin);
        insertAABB(node, &root);
    }
    // Tree is empty. Insert the node as the root.
    else {
        root = new AABBNode();
        root->setLeaf(aabb);
        root->updateAABB(margin);
    }
}

void AABBTree::insertAABB(AABBNode* node, AABBNode** parent_ptr) {
    AABBNode* parent = *parent_ptr;

    // If the parent is a leaf, we will split it into a node with two children,
    // 1 being the parent and the other being our node.
    if (parent->isLeaf()) {
        AABBNode* new_node = new AABBNode();
        new_node->parent = parent->parent;
        new_node->setBranch(parent, node);
        new_node->updateAABB(margin);
        *parent_ptr = new_node;
    }
    // Otherwise, recursively go into the child whose insertion will minimize
    // the increase in volume.
    else {
        const CollisionAABB aabb = node->aabb;
        const CollisionAABB p_child0 = parent->children[0]->aabb;
        const CollisionAABB p_child1 = parent->children[1]->aabb;

        const float volume_0_diff =
            p_child0.unionWith(aabb).volume() - aabb.volume();
        const float volume_1_diff =
            p_child1.unionWith(aabb).volume() - aabb.volume();

        // Insert into the child whose volume difference (after insertion) is
        // smaller
        if (volume_0_diff < volume_1_diff) {
            insertAABB(node, &parent->children[0]);
        } else {
            insertAABB(node, &parent->children[1]);
        }

        // After insertion, update the parent's AABB
        parent->updateAABB(margin);
    }
}

// RemoveAABB:
// Removes an AABB from the tree. Could be used to render a collider invalid,
// or to update a collider.
void AABBTree::remove(CollisionAABB* aabb) {
    // Do nothing if AABB has no node field. This means it is not
    // in the tree.
    if (aabb->node == nullptr)
        return;

    AABBNode* node = aabb->node;

    // Remove the link between the node and AABB
    node->data = nullptr;
    aabb->node = nullptr;

    // Remove the node from the tree
    removeAABB(node);
}

void AABBTree::removeAABB(AABBNode* node) {
    AABBNode* parent = node->parent;

    // If the node is the root, set the root to null. We are done.
    if (parent == nullptr) {
        root = nullptr;
        delete node;
    }
    // Otherwise, the node is somewhere in the tree. To remove it, we will merge
    // it's sibling with its parent and update the references.
    else {
        AABBNode* sibling = node->getSibling();

        // If the parent has no parent, the parent is the root. Replace the root\
        // with the sibling.
        if (parent->parent == nullptr) {
            root = sibling;
            sibling->parent = nullptr;
        }
        // If the parent has a parent, we want to make the sibling that parent's
        // child.
        else {
            AABBNode* grandparent = parent->parent;

            sibling->parent = grandparent;

            if (parent == grandparent->children[0])
                grandparent->children[0] = sibling;
            else
                grandparent->children[1] = sibling;
        }

        correctTreeAfterRemoval(sibling);

        delete node;
        delete parent;
    }
}

void AABBTree::correctTreeAfterRemoval(AABBNode* node) {
    node->updateAABB(margin);

    if (node->parent != nullptr)
        correctTreeAfterRemoval(node->parent);
}

// UpdateAABB:
// Updates the AABBTree, iterating through all AABBs and re-inserting all nodes
// that are outside of their node.
void AABBTree::update() {
    if (root == nullptr)
        return;

    if (root->isLeaf())
        root->updateAABB(margin);
    else {
        std::vector<CollisionAABB*> invalid;
        findInvalidBeforeUpdate(root, invalid);

        for (CollisionAABB* aabb : invalid) {
            remove(aabb);
            add(aabb);
        }
    }
}

void AABBTree::findInvalidBeforeUpdate(AABBNode* node,
                                       std::vector<CollisionAABB*>& invalid) {
    if (node == nullptr)
        return;

    // If the node is a leaf, check if its invalid. If invalid,
    if (node->isLeaf()) {
        // If the AABB within is not contained within the node, then we mark the
        // AABB contained within to be removed
        if (!node->aabb.contains(*node->data))
            invalid.push_back(node->data);
    } else {
        findInvalidBeforeUpdate(node->children[0], invalid);
        findInvalidBeforeUpdate(node->children[1], invalid);
    }
}

// ComputeColliderPairs:
// Broadphase that traverses the trees to find all AABBs (leaves) that are
// intersecting.
const std::vector<ColliderPair>& AABBTree::computeColliderPairs() {
    collider_pairs.clear();

    // Early-Out: If no root, or root is a leaf
    if (root == nullptr || root->isLeaf())
        return collider_pairs;

    // Traverse the tree
    findColliderPairs(root->children[0], root->children[1]);

    return collider_pairs;
}

void AABBTree::findColliderPairs(AABBNode* n1, AABBNode* n2) {
    // To find collider pairs, we compare the AABB boxes of the nodes.
    // Case 1 (Base Case): Both nodes are leaves.
    if (n1->isLeaf() && n2->isLeaf()) {
        // If the AABBs are intersecting, add to collision pair list.
        if (n1->data->intersects(*n2->data))
            collider_pairs.push_back(ColliderPair(n1->data, n2->data));
    }
    // Case 2: Both nodes are branches
    else if (!n1->isLeaf() && !n2->isLeaf()) {
        // Check if the AABBs are intersecting. If they are not, then we only
        // need to check for collisions within each node separately. Otherwise,
        // we need to check all combinations of children.
        if (n1->aabb.intersects(n2->aabb)) {
            findColliderPairs(n1->children[0], n1->children[1]);
            findColliderPairs(n1->children[0], n2->children[0]);
            findColliderPairs(n1->children[0], n2->children[1]);

            findColliderPairs(n1->children[1], n2->children[0]);
            findColliderPairs(n1->children[1], n2->children[1]);

            findColliderPairs(n2->children[0], n2->children[1]);
        } else {
            findColliderPairs(n1->children[0], n1->children[1]);
            findColliderPairs(n2->children[0], n2->children[1]);
        }
    }
    // Case 3: 1 node is a branch, 1 node is a leaf
    else {
        // Figure out which node is a leaf and which is a branch
        AABBNode* leaf = n1;
        AABBNode* branch = n2;

        if (n2->isLeaf()) {
            leaf = n2;
            branch = n1;
        }

        // Check if the nodes are intersecting. If they are, then we have to check for leaf collision with the branch's children.
        // Otherwise, we only check the branch separately.
        if (leaf->data->intersects(branch->aabb)) {
            findColliderPairs(leaf, branch->children[0]);
            findColliderPairs(leaf, branch->children[1]);
        }
        else {
            findColliderPairs(branch->children[0], branch->children[1]);
        }
    }
}

#if defined(DRAW_AABB_TREE)
void AABBTree::debugDrawTree() const {
    if (root != nullptr)
        debugDrawTreeHelper(root);
}

void AABBTree::debugDrawTreeHelper(AABBNode* cur) const {
    assert(cur != nullptr);

    if (cur->isLeaf())
        cur->aabb.debugDrawExtents(Color::Blue());
    else 
        cur->aabb.debugDrawExtents(Color::Red());

    if (!cur->isLeaf()) {
        debugDrawTreeHelper(cur->children[0]);
        debugDrawTreeHelper(cur->children[1]);
    }
}
#endif

} // namespace Physics
} // namespace Engine