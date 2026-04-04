#include "CFrame.h"

namespace Engine {
namespace Math {
CFrame::CFrame() : position(0, 0, 0), rotation() {}
CFrame::CFrame(const Vector3& pos, const Quaternion& rot) {
    position = pos;
    rotation = rot;
}
CFrame::~CFrame() = default;

const Vector3& CFrame::getPosition() const { return position; }
const Quaternion& CFrame::getRotation() const { return rotation; }

void CFrame::setPosition(const Vector3& newPosition) { position = newPosition; }
void CFrame::setRotation(const Quaternion& newRotation) {
    rotation = newRotation;
}

Matrix4 CFrame::generateLocalMatrix() const {
    return Matrix4(1, 0, 0, position.x, // Row 1
                   0, 1, 0, position.y, // Row 2
                   0, 0, 1, position.z, // Row 3
                   0, 0, 0, 1) *
           rotation.rotationMatrix4(); // Row 4
}

} // namespace Math
} // namespace Engine