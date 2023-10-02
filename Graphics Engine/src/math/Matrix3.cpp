#include "Matrix3.h"

namespace Engine
{
namespace Math
{
	Matrix3::Matrix3() : data{{0}} {}

	Matrix3::Matrix3(float c1, float c2, float c3,
					float c4, float c5, float c6,
					float c7, float c8, float c9) 
		: data{ {c1, c2, c3}, {c4, c5, c6}, {c7, c8, c9} }
	{}

	Matrix3 Matrix3::transpose() const
	{
		Matrix3 new_matrix;

		for (int row = 0; row < 3; row++)
		{
			for (int col = 0; col < 3; col++)
			{
				new_matrix[row][col] = data[col][row];
			}
		}

		return new_matrix;
	}

	Matrix3 Matrix3::inverse() const
	{
		Matrix3 inv;
		float det = determinant();

		for (int row = 0; row < 3; row++)
		{
			for (int col = 0; col < 3; col++)
			{
				// Take the adjoint and divide by the determinant 
				inv[row][col] = cofactor(col, row) / det;
			}
		}

		return inv;
	}
	
	float Matrix3::minor(int row, int col) const
	{
		int row_one = (row == 0 ? 1 : 0);
		int row_two = (row == 2 ? 1 : 2);

		int col_one = (col == 0 ? 1 : 0);
		int col_two = (col == 2 ? 1 : 2);

		return data[row_one][col_one] * data[row_two][col_two] - data[row_one][col_two] * data[row_two][col_one];
	}

	float Matrix3::cofactor(int row, int col) const
	{
		float m = minor(row, col);
		return (row + col) % 2 == 0 ? m : -m;
	}

	float Matrix3::trace() const
	{
		return data[0][0] + data[1][1] + data[2][2];
	}

	float Matrix3::determinant() const
	{
		return data[0][0] * cofactor(0, 0) + data[0][1] * cofactor(0, 1) + data[0][2] * cofactor(0, 2);
	}

	float* const Matrix3::operator[](int row)
	{
		return data[row];
	}

	Matrix3 Matrix3::operator*(Matrix3& matrix) const
	{
		Matrix3 new_matrix = Matrix3();

		for (int row = 0; row < 3; row++)
		{
			for (int col = 0; col < 3; col++)
			{
				float value = 0;
				
				for (int i = 0; i < 3; i++)
				{
					value += data[row][i] * matrix[i][col];
				}

				new_matrix[row][col] = value;
			}
		}

		return new_matrix;
	}

	Vector3 Matrix3::operator*(const Vector3& vector) const
	{
		float x = vector.x * (data[0][0] + data[1][0] + data[2][0]);
		float y = vector.y * (data[0][1] + data[1][1] + data[2][1]);
		float z = vector.z * (data[0][2] + data[1][2] + data[2][2]);

		return Vector3(x, y, z);
	}

	Matrix3 Matrix3::operator*(const float c) const
	{
		Matrix3 new_matrix = Matrix3();

		for (int row = 0; row < 3; row++)
		{
			for (int col = 0; col < 3; col++)
			{
				new_matrix[row][col] = data[row][col] * c;
			}
		}

		return new_matrix;
	}

	Matrix3 Matrix3::operator/(const float c) const
	{
		Matrix3 new_matrix = Matrix3();

		for (int row = 0; row < 3; row++)
		{
			for (int col = 0; col < 3; col++)
			{
				new_matrix[row][col] = data[row][col] / c;
			}
		}

		return new_matrix;
	}

} // Namespace Math
} // Namespace Engine