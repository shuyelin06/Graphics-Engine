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
    // Initialization by Column
    Matrix4(const Vector4& col1, const Vector4& col2, const Vector4& col3,
            const Vector4& col4);
    // Initialization by Row
    Matrix4(float, float, float, float, float, float, float, float, float,
            float, float, float, float, float, float, float);

    float (*getRawData(void))[4];

    float entry(int row, int col) const;
    void setEntry(int row, int col, float value);
    Vector4 column(int col) const;
    void setColumn(int col, const Vector4& column);

    Matrix4 transpose() const;
    Matrix4 inverse() const;

    float trace() const;
    float determinant() const;

    // Access the matrix as (column, row) coordinates
    float* const operator[](int);
    const float* const operator[](int) const;

    Matrix4 operator*(const Matrix4&) const;
    Vector4 operator*(const Vector4&) const;
    Matrix4 operator*(const float) const;
    Matrix4 operator/(const float) const;

    static Matrix4 Identity();
    static Matrix4 T_Scale(float x_scale, float y_scale, float z_scale);
    static Matrix4 T_Rotate(const Vector3& axis, float theta);
    static Matrix4 T_Translate(const Vector3& position);
    static Matrix4 T_Translate(float x, float y, float z);

  private:
    float minor(int col, int row) const;
    float cofactor(int col, int row) const;
};

} // Namespace Math
} // Namespace Engine
