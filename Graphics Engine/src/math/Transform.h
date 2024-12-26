#pragma once

#include "math/Matrix4.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"

namespace Engine {
namespace Math {
// Class Transform:
// Contains the data and methods regarding
// an object's transform
class Transform {
  private:
    Vector3 position_local; // X,Y,Z Local Position
    Quaternion rotation;    // Quaternion Rotation
    Vector3 scale;          // ScaleX, ScaleY, ScaleZ

  public:
    // Constructor
    Transform();

    // Get and set the transform properties
    const Vector3& getPosition() const;
    void setPosition(float x, float y, float z);
    void offsetPosition(float x, float y, float z);

    const Quaternion& getRotation() const;
    void setViewDirection(const Vector3& direction); // Set +Z in a Direction
    void lookAt(const Vector3& target);              // Sets Rotation to Target
    void setRotation(const Quaternion& quaterion);
    void setRotation(const Vector3& axis, float theta);
    void offsetRotation(const Vector3& axis, float theta);

    const Vector3& getScale() const;
    void setScale(float x, float y, float z);
    void offsetScale(float x, float y, float z);

    // Get directional vectors based on the rotation
    Vector3 forward(void) const;
    Vector3 backward(void) const;
    Vector3 right(void) const;
    Vector3 left(void) const;
    Vector3 up(void) const;
    Vector3 down(void) const;

    // Generates transformation matrices based off transform
    Matrix4 transformMatrix(void) const;

    Matrix4 scaleMatrix(void) const;
    Matrix4 rotationMatrix(void) const;
    Matrix4 translationMatrix(void) const;

    static Matrix4 GenerateTranslationMatrix(float x, float y, float z);

    static Matrix4 GenerateRotationMatrix(const Quaternion& q);
    static Matrix4 GenerateRotationMatrix(const Vector3& axis, float theta);
};
} // namespace Math
} // namespace Engine