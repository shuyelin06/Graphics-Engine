#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../Direct3D11.h"
#include "Texture.h"
#include "VertexStreamIDs.h"

#include "math/AABB.h"
#include "math/Color.h"
#include "math/Transform.h"
#include "math/Vector2.h"
#include "math/Vector3.h"

namespace Engine {
using namespace Math;
namespace Graphics {
typedef unsigned int UINT;

// --- Material ---
// Specifies renderable properties of a mesh. These properties are given in
// terms of texture coordinates into a texture atlas (which is used for many
// meshes)

// Normalized [0,1] coordinates into a material texture atlas.
struct TextureRegion {
    float x, y, width, height;
};

// Specifies renderable properties for a mesh.
struct Material {
    TextureRegion tex_region;

    float diffuse_factor; // Roughness

  public:
    // Default material settings
    Material();
};

// --- Mesh ---
// Specifies a mesh, which is a collection of vertices that has the same
// material (renderable configuration). Vertices are stored in separate vertex
// streams, so that they have an easier time being passed as input into shaders.

// TODO: MeshPool will let us group meshes into shared buffers
struct Mesh;
struct MeshPool {
  private:
    friend class MeshBuilder;
    // Default Constructor, used by MeshBuilder to create
    // a MeshPool that is not mappable
    MeshPool();

  public:
    // Layout, i.e. the streams that the pool stores
    uint16_t layout;
    // Tracks whether or not the buffers are mappable
    bool mappable;

    // Index / Vertex Buffers
    ID3D11Buffer* ibuffer;
    uint32_t triangle_size;     // # Tris Currently in the Buffer
    uint32_t triangle_capacity; // # Tris the Buffer can Hold

    ID3D11Buffer* vbuffers[BINDABLE_STREAM_COUNT];
    uint32_t vertex_size;     // # Vertices Currently in the Buffer
    uint32_t vertex_capacity; // # Vertices the Buffer can Hold

    // CPU-Side buffers, for the clean and compact operation.
    // Only allocated if mappable == true
    // Tracks the Meshes Allocated to this Pool.
    std::optional<std::vector<Mesh*>> meshes;

    uint8_t* cpu_ibuffer;
    uint8_t* cpu_vbuffers[BINDABLE_STREAM_COUNT];

  public:
    MeshPool(ID3D11Device* device, uint16_t layout, uint32_t triangle_max,
             uint32_t vertex_max);
    ~MeshPool();

    // Only possible if mappable == true.
    // Iterates through the meshes allocated to this pool, and compacts them
    // to remove fragmentation.
    // First call cleanAndCompact(), then upload this new data to the GPU
    void cleanAndCompact();
    void updateGPUResources(ID3D11DeviceContext* context);
};

struct Mesh {
  private:
    friend class MeshBuilder;
    Mesh(MeshPool* pool);

  public:
    // MeshPool (Collection of Buffers) Containing the Mesh.
    MeshPool* buffer_pool;
    uint16_t layout;

    // Vertex / Index Starts and Offsets into the MeshPool
    uint32_t vertex_start;
    uint32_t num_vertices;

    uint32_t triangle_start;
    uint32_t num_triangles;

    // AABB for the Mesh
    Math::AABB aabb;

    // Renderable Properties
    Material material;

    ~Mesh();
};

// --- Node ---
// Defines a local transform in space. Nodes can have children, and the
// transforms of these children are influenced by their parents.
// Nodes are used for skinning and animations.
struct Node {
    Transform transform;
    Matrix4 m_local;

    Node* parent;
    std::vector<Node*> children;

    // Tracks whether or not the node's transform has been updated already
    bool update_flag;

    Node();
    Node(const Transform& transform);
};

// --- Skin ---
// Associates a set of nodes with inverse bind matrices. These matrices
// will transform a vertex to the joint's local space, which lets us
// apply the joint's transform to that vertex.
// So, for any vertex, we do the following:
// 1) Apply the inverse bind matrix to transform to local joint space
// 2) Apply the joint transform * a weight (optional)
// 3) Apply (inverse bind)^-1 matrix to retransform back to model space
// For multiple joints, a vertex will also store weights determining how much
// that joint matrix influences the vertex
struct SkinJoint {
    const Node* node;
    Matrix4 m_transform;

    const Matrix4 m_inverse_bind;

    SkinJoint(const Node* node, const Matrix4& m_inverse_bind);

    Matrix4 getTransform(const Node* node) const;
};

// --- Animation ---
// An animation defines a set of local orientations for nodes in an asset,
// which change over time.
// The base construct for an animation is the AnimationState, which defines
// how one node's local properties change within a time frame t in [0,1].
enum LocalStateType { ANIMATION_POSITION, ANIMATION_ROTATION, ANIMATION_SCALE };
class LocalState {
  private:
    float x, y, z, w;
    float time;

  public:
    LocalState();

    Vector3 position() const;
    Quaternion rotation() const;
    Vector3 scale() const;

    void setData(const Vector4& data);
    void setPosition(const Vector3& pos);
    void setRotation(const Quaternion& rot);
    void setScale(const Vector3& scale);

    float getTime() const;
    void setTime(float time);
};

class AnimationState {
  private:
    Node* target_node;
    LocalStateType state_type;

    std::vector<LocalState> local_states;

  public:
    AnimationState(Node* target_node, LocalStateType state_type);

    void addState(const LocalState& state);
    void normalizeTimes(); // Normalize times to [0,1]

    LocalStateType getType() const;
    Node* getTargetNode() const;
    LocalState stateAtTime(float time) const;
};

class Animation {
  private:
    std::vector<AnimationState*> states;

  public:
    Animation();

    AnimationState& newAnimationState(Node* target_node,
                                      LocalStateType state_type);

    void updateTransformsforTime(float time) const;
};

// --- Asset ---
// Represents a renderable entity. Assets are composed of multiple
// meshes, each of which can has a material. Together, these meshes
// compose one renderable entity.
// The placement of meshes within an asset is defined by node classes.
// This allows for animations within the asset.
class Asset {
  private:
    // Meshes that the asset is made up of. A mesh defines a renderable
    // collection of triangles in the asset.
    std::vector<Mesh*> meshes;

    // Nodes in the asset. Allows mesh skinning and animations.
    std::vector<Node*> nodes;

    // Skin joints. Skinned meshes can refer to these joints
    // for vertex skinning
    std::vector<SkinJoint> skin;

    // Animations in the mesh.
    std::vector<Animation*> animations;

  public:
    Asset();
    ~Asset();

    // Asset Creation.
    // Upon adding to an asset, the index of the resource added will be
    // returned.
    void addSkinJoint(const Node* node, const Matrix4& m_inverse_bind);

    UINT addMesh(Mesh* mesh);
    UINT addNode(Node* node);
    UINT addAnimation(Animation* animation);

    // Asset Modification
    // Apply run-time modifications to the asset
    void applyAnimationAtTime(UINT animation_index, float time) const;

    // Asset Accessing.
    // Retrieve data from the asset for rendering
    const std::vector<Mesh*>& getMeshes() const;
    const Mesh* getMesh(UINT index) const;

    const std::vector<Node*>& getNodes() const;
    const Node* getNode(UINT index) const;

    const std::vector<SkinJoint>& getSkinJoints() const;
    bool isSkinned() const;
};

} // namespace Graphics
} // namespace Engine