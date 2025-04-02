#include "Asset.h"

#include <assert.h>

namespace Engine {
using namespace Math;

namespace Graphics {
// --- Material ---
Material::Material() { diffuse_factor = 0.5f; }

// --- Mesh ---
// ...

// --- Node ---
Node::Node() : children(0), parent(nullptr) { transform = Transform(); }
Node::Node(const Transform& _transform) : children(0), parent(nullptr) {
    transform = _transform;
}

// --- Skin ---
SkinJoint::SkinJoint(const Node* _node, const Matrix4& _m_inverse_bind)
    : node(_node), m_inverse_bind(_m_inverse_bind) {}

Matrix4 SkinJoint::getTransform(const Node* node) const {
    if (node == nullptr)
        return Matrix4::Identity();
    else {
        Matrix4 cur_node = node->transform.transformMatrix();
        return getTransform(node->parent) * cur_node;
    }
}

// --- Animation ---
// Local State Accessors / Setters.
// A local state could represent a position, rotation, or
// scale.
LocalState::LocalState() {
    x = y = z = w = 0.f;
    time = 0.f;
}

Vector3 LocalState::position() const { return Vector3(x, y, z); }
Quaternion LocalState::rotation() const {
    return Quaternion(Vector3(x, y, z), w);
}
Vector3 LocalState::scale() const { return Vector3(x, y, z); }

void LocalState::setData(const Vector4& data) {
    x = data.x;
    y = data.y;
    z = data.z;
    w = data.w;
}
void LocalState::setPosition(const Vector3& pos) {
    x = pos.x;
    y = pos.y;
    z = pos.z;
}
void LocalState::setRotation(const Quaternion& rot) {
    const Vector3& im = rot.getIm();
    const float r = rot.getR();

    x = im.x;
    y = im.y;
    z = im.z;
    w = r;
}
void LocalState::setScale(const Vector3& scale) {
    x = scale.x;
    y = scale.y;
    z = scale.z;
}

float LocalState::getTime() const { return time; }
void LocalState::setTime(float _time) { time = _time; }

// An animation stores a collection of local states over time,
// and defines the property of the local state as well as the target node.
AnimationState::AnimationState(Node* _target_node, LocalStateType _state_type) {
    target_node = _target_node;
    state_type = _state_type;

    local_states.resize(0);
}

void AnimationState::addState(const LocalState& _state) {
    local_states.push_back(_state);
}

void AnimationState::normalizeTimes() {
    float minimum = 0.f, maximum = 0.f;

    for (const LocalState& state : local_states) {
        minimum = min(minimum, state.getTime());
        maximum = max(maximum, state.getTime());
    }

    for (LocalState& state : local_states) {
        const float new_time =
            (state.getTime() + minimum) / (maximum - minimum);
        state.setTime(new_time);
    }
}

LocalStateType AnimationState::getType() const { return state_type; }
Node* AnimationState::getTargetNode() const { return target_node; }

LocalState AnimationState::stateAtTime(float time) const {
    if (local_states.size() == 1)
        return local_states[0];

    const LocalState* one = nullptr;
    const LocalState* two = nullptr;

    // Find the two local states just before and after our time.
    for (int i = 0; i < local_states.size(); i++) {
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
    if (state_type == ANIMATION_POSITION) {
        const Vector3 pos =
            Vector3::Lerp(one->position(), two->position(), rel_time);
        output.setPosition(pos);
    } else if (state_type == ANIMATION_SCALE) {
        const Vector3 scale =
            Vector3::Lerp(one->scale(), two->scale(), rel_time);
        output.setPosition(scale);
    } else if (state_type == ANIMATION_ROTATION) {
        const Quaternion rot =
            Quaternion::Slerp(one->rotation(), two->rotation(), rel_time);
        output.setRotation(rot);
    }

    return output;
}

Animation::Animation() { states.resize(0); }

AnimationState& Animation::newAnimationState(Node* target_node,
                                             LocalStateType state_type) {
    AnimationState* state = new AnimationState(target_node, state_type);
    states.push_back(state);
    return *state;
}

static void updateToParent(Node* node) {
    if (node->parent == nullptr)
        node->m_local = node->transform.transformMatrix();
    else {
        updateToParent(node->parent);
        node->m_local = node->parent->m_local * node->m_local;
    }
}
void Animation::updateTransformsforTime(float time) const {
    for (const AnimationState* state : states) {
        Node* target = state->getTargetNode();
        LocalState local_state = state->stateAtTime(time);

        switch (state->getType()) {
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
    for (AnimationState* state : states) {
        updateToParent(state->getTargetNode());
    }
}

// --- Asset ---
Asset::Asset() = default;
Asset::~Asset() = default;

// Create an Asset
void Asset::addSkinJoint(const Node* node, const Matrix4& m_inverse_bind) {
    skin.push_back(SkinJoint(node, m_inverse_bind));
}

UINT Asset::addMesh(Mesh* mesh) {
    const UINT index = meshes.size();
    meshes.push_back(mesh);
    return index;
}

UINT Asset::addNode(Node* node) {
    const UINT index = nodes.size();
    nodes.push_back(node);
    return index;
}

UINT Asset::addAnimation(Animation* animation) {
    const UINT index = animations.size();
    animations.push_back(animation);
    return index;
}

// Modify an Asset
void Asset::applyAnimationAtTime(UINT animation_index, float time) const {
    const Animation* animation = animations[animation_index];

    animation->updateTransformsforTime(time);
}

// Access an Asset
const std::vector<Mesh*>& Asset::getMeshes() const { return meshes; }
const Mesh* Asset::getMesh(UINT index) const { return meshes[index]; }

const std::vector<Node*>& Asset::getNodes() const { return nodes; }
const Node* Asset::getNode(UINT index) const { return nodes[index]; }

const std::vector<SkinJoint>& Asset::getSkinJoints() const { return skin; }
bool Asset::isSkinned() const { return skin.size() != 0; }

} // namespace Graphics
} // namespace Engine