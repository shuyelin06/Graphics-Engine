#pragma once

#include <vector>

#include "math/Vector3.h"
#include "math/Vector2.h"

#if defined(_DEBUG)
#include "rendering/VisualDebug.h"
#endif

namespace Engine {
using namespace Math;

namespace Datamodel {
// Tokens in the Context Free Grammar. The CFG for a tree is:
// T -> tT | tbTlT | l 
// Where
// t: Trunk; Some stretch of wood on the tree
// b: Branch; Some offshoot from the current wood
// l: Leaf; denotes the end of a branch, or a trunk
enum TreeToken { TRUNK, BRANCH, LEAF };
struct TreeStructure {
    TreeToken token;
    
    // Extra data that will go with the tree's token
    float trunk_length;

    float branch_angle_phi;
    float branch_angle_theta;

    float leaf_density;
};
// Class TreeGenerator:
// A procedural generator for trees. Uses context free grammars
// to do this.
class TreeGenerator {
  private:
    std::vector<TreeStructure> grammar;

    // Fields that influence generation probabilities
    int branch_depth; // How many branches we're in right now

  public:
    TreeGenerator();
    ~TreeGenerator();

    const std::vector<TreeStructure>& getTree();

    // Randomly creates a tree using recursion and the CFG's rules
    void generateTree();

#if defined(_DEBUG)
    void debugDrawTree();
#endif

  private:
    void generateTreeHelper();
    
    float trunkProbability() const;
    float branchProbability() const;
    float leafProbability() const;

    void addTrunk();
    void addLeaf();
    void addBranch();

#if defined(_DEBUG)
    int debugDrawTreeHelper(int index, const Vector3& position, const Vector2& direction);
#endif
};

} // namespace Datamodel
} // namespace Engine