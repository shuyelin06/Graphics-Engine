#pragma once

#include "Vector3.h"
#include "Vector4.h"

namespace Engine {
namespace Math {

// Matrix4
// Contains methods and data for a 4x4 matrix
// in column major order.
class Matrix4 {
  private:
    // Values are stored by column for more predictable
    // memory access patterns.
    float data[4][4];

  public:
    Matrix4();
    Matrix4(const Vector4& col1, const Vector4& col2, const Vector4& col3,
            const Vector4& col4);
    Matrix4(float, float, float, float, float, float, float, float, float,
            float, float, float, float, float, float, float);

    static Matrix4 Identity();

    float (*getRawData(void))[4];
    
    Vector4 column(int col) const;
    void setColumn(int col, const Vector4& column);

    Matrix4 transpose() const;
    Matrix4 inverse() const;

    float trace() const;
    float determinant() const;

    float minor(int col, int row) const;
    float cofactor(int col, int row) const;

    // Access the matrix as (column, row) coordinates
    float* const operator[](int);
    const float* const operator[](int) const;

    Matrix4 operator*(const Matrix4&) const;
    Vector4 operator*(const Vector4&) const;
    Matrix4 operator*(const float) const;
    Matrix4 operator/(const float) const;
};

} // Namespace Math
} // Namespace Engine
