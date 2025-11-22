#pragma once

#include <memory>
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
    // Meshes Allocated to this Pool.
    // This pool is in charge of deallocating them.
    std::vector<std::shared_ptr<Mesh>> meshes;
    // Layout, i.e. the streams that the pool stores
    uint16_t layout;

    // CPU-Side Buffers for the Pool.
    std::unique_ptr<uint8_t[]> cpu_ibuffer;
    std::unique_ptr<uint8_t[]> cpu_vbuffers[BINDABLE_STREAM_COUNT];

    // GPU-Side Index / Vertex Buffers
    bool has_gpu_resources;

    ID3D11Buffer* ibuffer;
    uint32_t triangle_size;     // # Tris Currently in the Buffer
    uint32_t triangle_capacity; // # Tris the Buffer can Hold

    ID3D11Buffer* vbuffers[BINDABLE_STREAM_COUNT];
    uint32_t vertex_size;     // # Vertices Currently in the Buffer
    uint32_t vertex_capacity; // # Vertices the Buffer can Hold

  public:
    MeshPool(uint16_t layout, uint32_t triangle_max, uint32_t vertex_max);
    ~MeshPool();

    // Cleans and compacts the GPU-side data.
    void cleanAndCompact();

    // Works with the GPU resources.
    // To update, you must have created the GPU resources first.
    void createGPUResources(ID3D11Device* device);
    void updateGPUResources(ID3D11DeviceContext* context);
};

struct Mesh {
    Mesh(MeshPool* pool);
    ~Mesh();

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
};

// TODO: Move animation stuff out.
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
    std::vector<std::shared_ptr<Mesh>> meshes;

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

    UINT addMesh(std::shared_ptr<Mesh>& mesh);
    UINT addNode(Node* node);
    UINT addAnimation(Animation* animation);

    // Asset Modification
    // Apply run-time modifications to the asset
    void applyAnimationAtTime(UINT animation_index, float time) const;

    // Asset Accessing.
    // Retrieve data from the asset for rendering
    const std::vector<std::shared_ptr<Mesh>>& getMeshes() const;
    const Mesh* getMesh(UINT index) const;

    const std::vector<Node*>& getNodes() const;
    const Node* getNode(UINT index) const;

    const std::vector<SkinJoint>& getSkinJoints() const;
    bool isSkinned() const;
};

} // namespace Graphics
} // namespace Engine