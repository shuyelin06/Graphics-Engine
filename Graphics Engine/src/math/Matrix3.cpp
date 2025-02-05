#include "Matrix3.h"

namespace Engine {
namespace Math {
Matrix3::Matrix3() : data{{0}} {}

Matrix3::Matrix3(float c1, float c2, float c3, float c4, float c5, float c6,
                 float c7, float c8, float c9)
    : data{{c1, c4, c7}, {c2, c5, c8}, {c3, c6, c9}} {}

Matrix3 Matrix3::transpose() const {
    Matrix3 new_matrix;

    for (int col = 0; col < 3; col++) {
        for (int row = 0; row < 3; row++) {
            new_matrix[col][row] = data[row][col];
        }
    }

    return new_matrix;
}

Matrix3 Matrix3::inverse() const {
    Matrix3 inv;
    float det = determinant();

    for (int col = 0; col < 3; col++) {
        for (int row = 0; row < 3; row++) {
            // Take the adjoint and divide by the determinant
            inv[col][row] = cofactor(row, col) / det;
        }
    }

    return inv;
}

float Matrix3::minor(int col, int row) const {
    const int col_one = (col == 0 ? 1 : 0);
    const int col_two = (col == 2 ? 1 : 2);
    const int row_one = (row == 0 ? 1 : 0);
    const int row_two = (row == 2 ? 1 : 2);

    return data[col_one][row_one] * data[col_two][row_two] -
           data[col_two][row_one] * data[col_one][row_two];
}

float Matrix3::cofactor(int col, int row) const {
    float m = minor(col, row);
    return (row + col) % 2 == 0 ? m : -m;
}

float Matrix3::trace() const { return data[0][0] + data[1][1] + data[2][2]; }

float Matrix3::determinant() const {
    return data[0][0] * cofactor(0, 0) + data[0][1] * cofactor(0, 1) +
           data[0][2] * cofactor(0, 2);
}

float* const Matrix3::operator[](int col) { return data[col]; }

Matrix3 Matrix3::operator*(Matrix3& matrix) const {
    Matrix3 new_matrix = Matrix3();

    for (int col = 0; col < 3; col++) {
        for (int row = 0; row < 3; row++) {
            float value = 0;

            for (int i = 0; i < 3; i++)
                value += data[i][row] * matrix[col][i];

            new_matrix[col][row] = value;
        }
    }

    return new_matrix;
}

Vector3 Matrix3::operator*(const Vector3& vector) const {
    const float x =
        vector.x * data[0][0] + vector.y * data[1][0] + vector.z * data[2][0];
    const float y =
        vector.x * data[0][1] + vector.y * data[1][1] + vector.z * data[2][1];
    const float z =
        vector.x * data[0][2] + vector.y * data[1][2] + vector.z * data[2][2];

    return Vector3(x, y, z);
}

Matrix3 Matrix3::operator*(const float c) const {
    Matrix3 new_matrix = Matrix3();

    for (int col = 0; col < 3; col++) {
        for (int row = 0; row < 3; row++) {
            new_matrix[col][row] = data[col][row] * c;
        }
    }

    return new_matrix;
}

Matrix3 Matrix3::operator/(const float c) const {
    Matrix3 new_matrix = Matrix3();

    for (int col = 0; col < 3; col++) {
        for (int row = 0; row < 3; row++) {
            new_matrix[col][row] = data[col][row] / c;
        }
    }

    return new_matrix;
}

} // Namespace Math
} // Namespace Engine