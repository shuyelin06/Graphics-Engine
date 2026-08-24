#include "Mesh.h"

#include <assert.h>

namespace Engine
{
using namespace Math;

namespace Graphics
{
// --- Mesh ---
MeshPool::MeshPool()
{
    layout = VertexLayout();

    ibuffer = NULL;
    cpu_ibuffer = NULL;
    index_size = index_capacity = 0;

    memset(cpu_vbuffers, 0, sizeof(uint8_t*) * BINDABLE_STREAM_COUNT);
    vertex_size = vertex_capacity = 0;
}

MeshPool::MeshPool(VertexLayout _layout, uint32_t tri_size, uint32_t v_size)
    : meshes()
{
    layout = _layout;
    has_gpu_resources = false;

    // Create my index buffer
    ibuffer = nullptr;

    index_size = 0;
    index_capacity = tri_size * 3;

    cpu_ibuffer = std::make_unique<uint8_t[]>(index_capacity * sizeof(UINT));

    // Create my CPU-side vertex buffers
    memset(cpu_vbuffers, 0, sizeof(uint8_t*) * BINDABLE_STREAM_COUNT);

    vertex_size = 0;
    vertex_capacity = v_size;

    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++)
    {
        if (layout.hasVertexStream((VertexDataStream)i))
        {
            cpu_vbuffers[i] = std::make_unique<uint8_t[]>(
                vertex_capacity *
                VertexLayout::VertexStreamStride((VertexDataStream)i));
        }
    }
}

void MeshPool::cleanAndCompact()
{
    std::vector<std::shared_ptr<Geometry>> meshesStrong;
    meshesStrong.reserve(meshes.size());

    // Iterate through the mesh pointers, removing pointers with only one
    // reference. These are meshes that are no longer being used anywhere else.
    std::vector<std::weak_ptr<Geometry>>::iterator iter;
    for (iter = meshes.begin(); iter != meshes.end();)
    {
        std::shared_ptr<Geometry> mesh = (*iter).lock();
        if (!mesh)
        {
            iter = meshes.erase(iter);
        }
        else
        {
            meshesStrong.emplace_back(std::move(mesh));
            ++iter;
        }
    }

    // Iterate through meshes, removing fragmentation in the index buffer
    // We do this on the CPU side first.
    int head = 0;
    for (int i = 0; i < meshesStrong.size(); i++)
    {
        Geometry* mesh = meshesStrong[i].get();

        if (head != mesh->indexOffset)
        {
            std::memmove(cpu_ibuffer.get() + head,
                         cpu_ibuffer.get() + mesh->indexOffset,
                         sizeof(UINT) * mesh->indexCount);
            mesh->indexOffset = head;
        }

        head += mesh->indexOffset;
    }

    index_size = head;

    // Remove fragmentation in the vertex buffers on the CPU-side
    head = 0;

    for (int i = 0; i < meshesStrong.size(); i++)
    {
        Geometry* mesh = meshesStrong[i].get();

        if (head != mesh->vertexOffset)
        {
            for (int i = 0; i < BINDABLE_STREAM_COUNT; i++)
            {
                if (cpu_vbuffers[i] != nullptr)
                {
                    const UINT STRIDE =
                        VertexLayout::VertexStreamStride((VertexDataStream)i);
                    std::memmove(cpu_vbuffers[i].get() + STRIDE * head,
                                 cpu_vbuffers[i].get() +
                                     STRIDE * mesh->vertexOffset,
                                 STRIDE * mesh->vertexCount);
                }
            }
            mesh->vertexOffset = head;
        }

        head += mesh->vertexCount;
    }

    vertex_size = head;
}

void MeshPool::createGPUResources(Device* device)
{
    has_gpu_resources = true;

    ibuffer = device->createBuffer("Index Buffer", BufferType::Index,
                                   index_capacity * 3 * sizeof(UINT),
                                   nullptr, true);

    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++)
    {
        if (layout.hasVertexStream((VertexDataStream)i))
        {
            vbuffers[i] = device->createBuffer(
                "Vertex Buffer", BufferType::Vertex,
                vertex_capacity *
                    VertexLayout::VertexStreamStride((VertexDataStream)i),
                nullptr, true);
        }
    }
}

void MeshPool::updateGPUResources(DeviceContext* context)
{
    assert(has_gpu_resources);

    HRESULT result;

    // Copy index buffer to the GPU
    context->updateBuffer(ibuffer, cpu_ibuffer.get(),
                          index_capacity * sizeof(UINT));

    // Copy vertex data to the GPU
    for (int i = 0; i < BINDABLE_STREAM_COUNT; i++)
    {
        if (vbuffers[i] != nullptr)
        {
            context->updateBuffer(
                vbuffers[i], cpu_vbuffers[i].get(),
                vertex_capacity *
                    VertexLayout::VertexStreamStride((VertexDataStream)i));
        }
    }
}

MeshPool::~MeshPool() {}

// --- Node ---
Node::Node()
    : children(0)
    , parent(nullptr)
{
    transform = Transform();
}
Node::Node(const Transform& _transform)
    : children(0)
    , parent(nullptr)
{
    transform = _transform;
}

// --- Skin ---
SkinJoint::SkinJoint(const Node* _node, const Matrix4& _m_inverse_bind)
    : node(_node)
    , m_inverse_bind(_m_inverse_bind)
{
}

Matrix4 SkinJoint::getTransform(const Node* node) const
{
    if (node == nullptr)
        return Matrix4::Identity();
    else
    {
        Matrix4 cur_node = node->transform.transformMatrix();
        return getTransform(node->parent) * cur_node;
    }
}

// --- Animation ---
// Local State Accessors / Setters.
// A local state could represent a position, rotation, or
// scale.
LocalState::LocalState()
{
    x = y = z = w = 0.f;
    time = 0.f;
}

Vector3 LocalState::position() const { return Vector3(x, y, z); }
Quaternion LocalState::rotation() const
{
    return Quaternion(Vector3(x, y, z), w);
}
Vector3 LocalState::scale() const { return Vector3(x, y, z); }

void LocalState::setData(const Vector4& data)
{
    x = data.x;
    y = data.y;
    z = data.z;
    w = data.w;
}
void LocalState::setPosition(const Vector3& pos)
{
    x = pos.x;
    y = pos.y;
    z = pos.z;
}
void LocalState::setRotation(const Quaternion& rot)
{
    const Vector3& im = rot.getIm();
    const float r = rot.getR();

    x = im.x;
    y = im.y;
    z = im.z;
    w = r;
}
void LocalState::setScale(const Vector3& scale)
{
    x = scale.x;
    y = scale.y;
    z = scale.z;
}

float LocalState::getTime() const { return time; }
void LocalState::setTime(float _time) { time = _time; }

// An animation stores a collection of local states over time,
// and defines the property of the local state as well as the target node.
AnimationState::AnimationState(Node* _target_node, LocalStateType _state_type)
{
    target_node = _target_node;
    state_type = _state_type;

    local_states.resize(0);
}

void AnimationState::addState(const LocalState& _state)
{
    local_states.push_back(_state);
}

void AnimationState::normalizeTimes()
{
    float minimum = 0.f, maximum = 0.f;

    for (const LocalState& state : local_states)
    {
        minimum = min(minimum, state.getTime());
        maximum = max(maximum, state.getTime());
    }

    for (LocalState& state : local_states)
    {
        const float new_time =
            (state.getTime() + minimum) / (maximum - minimum);
        state.setTime(new_time);
    }
}

LocalStateType AnimationState::getType() const { return state_type; }
Node* AnimationState::getTargetNode() const { return target_node; }

LocalState AnimationState::stateAtTime(float time) const
{
    if (local_states.size() == 1)
        return local_states[0];

    const LocalState* one = nullptr;
    const LocalState* two = nullptr;

    // Find the two local states just before and after our time.
    for (int i = 0; i < local_states.size(); i++)
    {
        if (local_states[i].getTime() < time)
            one = &local_states[i];
        if (local_states[local_states.size() - 1 - i].getTime() > time)
            two = &local_states[local_states.size() - 1 - i];
    }

    if (one == nullptr || two == nullptr)
        return LocalState();

    LocalState output;

    const float rel_time =
        (time - one->getTime()) / (two->getTime() - one->getTime());
    if (state_type == ANIMATION_POSITION)
    {
        const Vector3 pos =
            Vector3::Lerp(one->position(), two->position(), rel_time);
        output.setPosition(pos);
    }
    else if (state_type == ANIMATION_SCALE)
    {
        const Vector3 scale =
            Vector3::Lerp(one->scale(), two->scale(), rel_time);
        output.setPosition(scale);
    }
    else if (state_type == ANIMATION_ROTATION)
    {
        const Quaternion rot =
            Quaternion::Slerp(one->rotation(), two->rotation(), rel_time);
        output.setRotation(rot);
    }

    return output;
}

Animation::Animation() { states.resize(0); }

AnimationState& Animation::newAnimationState(Node* target_node,
                                             LocalStateType state_type)
{
    AnimationState* state = new AnimationState(target_node, state_type);
    states.push_back(state);
    return *state;
}

static void updateToParent(Node* node)
{
    if (node->parent == nullptr)
        node->m_local = node->transform.transformMatrix();
    else
    {
        updateToParent(node->parent);
        node->m_local = node->parent->m_local * node->m_local;
    }
}
void Animation::updateTransformsforTime(float time) const
{
    for (const AnimationState* state : states)
    {
        Node* target = state->getTargetNode();
        LocalState local_state = state->stateAtTime(time);

        switch (state->getType())
        {
        case ANIMATION_POSITION:
            target->transform.setPosition(local_state.position());
            break;

        case ANIMATION_SCALE:
            target->transform.setScale(local_state.scale());
            break;

        case ANIMATION_ROTATION:
            target->transform.setRotation(local_state.rotation());
            break;
        }
    }

    // Update node transform matrices
    for (AnimationState* state : states)
    {
        updateToParent(state->getTargetNode());
    }
}

// --- Asset ---
Asset::Asset() = default;
Asset::~Asset() = default;

// Create an Asset
void Asset::addSkinJoint(const Node* node, const Matrix4& m_inverse_bind)
{
    skin.push_back(SkinJoint(node, m_inverse_bind));
}

UINT Asset::addNode(Node* node)
{
    const UINT index = nodes.size();
    nodes.push_back(node);
    return index;
}

UINT Asset::addAnimation(Animation* animation)
{
    const UINT index = animations.size();
    animations.push_back(animation);
    return index;
}

// Modify an Asset
void Asset::applyAnimationAtTime(UINT animation_index, float time) const
{
    const Animation* animation = animations[animation_index];

    animation->updateTransformsforTime(time - floorf(time));
}

// Access an Asset
const std::vector<Node*>& Asset::getNodes() const { return nodes; }
const Node* Asset::getNode(UINT index) const { return nodes[index]; }

const std::vector<SkinJoint>& Asset::getSkinJoints() const { return skin; }
bool Asset::isSkinned() const { return skin.size() != 0; }

} // namespace Graphics
} // namespace Engine