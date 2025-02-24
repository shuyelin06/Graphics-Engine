#pragma once

#include "Vector3.h"

namespace Engine {
namespace Math {

// Matrix3
// Contains methods and data for a 3x3 matrix.
// This is internally stored as a column-major matrix.
class Matrix3 {
  protected:
    float data[3][3];

  public:
    Matrix3();
    Matrix3(float, float, float, float, float, float, float, float, float);

    Vector3 column(int index) const;

    Matrix3 transpose() const;
    Matrix3 inverse() const;

    float minor(int col, int row) const;
    float cofactor(int col, int row) const;

    float trace() const;
    float determinant() const;

    float* const operator[](int);

    Matrix3 operator*(Matrix3&) const;
    Vector3 operator*(const Vector3&) const;
    Matrix3 operator*(const float) const;
    Matrix3 operator/(const float) const;
};

} // Namespace Math
} // Namespace Engine