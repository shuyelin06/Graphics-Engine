#pragma once

#include "Matrix4.h"
#include "Quaternion.h"
#include "Vector3.h"

namespace Engine {
namespace Math {
// Coordinate Frame:
// Position (Vector3) + Rotation (Quaternion)
class CFrame {
  private:
    Vector3 position;
    Quaternion rotation;

  public:
    CFrame();
    CFrame(const Vector3& pos, const Quaternion& rot);
    ~CFrame();

    const Vector3& getPosition() const;
    const Quaternion& getRotation() const;

    void setPosition(const Vector3& newPosition);
    void setRotation(const Quaternion& newRotation);

    Matrix4 generateLocalMatrix() const;
};

} // namespace Math
} // namespace Engine