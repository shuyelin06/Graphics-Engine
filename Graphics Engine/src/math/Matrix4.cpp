#include "Matrix4.h"
#include "Matrix3.h"

namespace Engine
{
namespace Math
{
	// Constructors:
	// Initialize a 4x4 matrix with only 0s as entries
	Matrix4::Matrix4()
		: data{ {0} } 
	{
	}

	// Initialize a matrix with given vectors as its column vectors.
	Matrix4::Matrix4(const Vector4& col1, const Vector4& col2, const Vector4& col3, const Vector4& col4)
		: data{
			{ col1.x, col1.y, col1.z, col1.w },
			{ col2.x, col2.y, col2.z, col2.w },
			{ col3.x, col3.y, col3.z, col3.w },
			{ col4.x, col4.y, col4.z, col4.w }
		}
	{
	}

	// TODO
	// Initialize a 4x4 matrix with any 16 values as entries. 
	Matrix4::Matrix4(float c1, float c2, float c3, float c4,
					float c5, float c6, float c7, float c8,
					float c9, float c10, float c11, float c12,
					float c13, float c14, float c15, float c16) 
		: data{ 
			{ c1, c5, c9, c13 }, 
			{ c2, c6, c10, c14 }, 
			{ c3, c7, c11, c15 }, 
			{ c4, c8, c12, c16 } 
		} 
	{}

	// Identity 
	// Returns the 4x4 identity matrix
	Matrix4 Matrix4::identity()
	{
		return Matrix4( 1, 0, 0, 0,
						0, 1, 0, 0,
						0, 0, 1, 0,
						0, 0, 0, 1 );
	}

	/* --- Matrix Operations --- */
	float (*Matrix4::getRawData(void))[4]
	{
		return data;
	}

	// Tranpose: 
	// Returns the transpose of the matrix
	Matrix4 Matrix4::tranpose() const
	{
		Matrix4 matrix_transpose;

		for (int col = 0; col < 4; col++)
			for (int row = 0; row < 4; row++)
				matrix_transpose[col][row] = data[row][col];

		return matrix_transpose;
	}

	// Inverse:
	// Takes and returns the inverse of a matrix,
	// using the adjugate method
	Matrix4 Matrix4::inverse() const
	{
		Matrix4 matrix_inverse;

		// Find determinant
		const float det = determinant();

		// Build inverse matrix by simultaneously taking
		// the transpose of the cofactor matrix
		// and dividing it by the determinant
		for (int col = 0; col < 4; col++)
			for (int row = 0; row < 4; row++)
				matrix_inverse[col][row] = cofactor(row, col) / det;

		return matrix_inverse;
	}

	// Trace:
	// Takes the trace of the matrix, the sum of the
	// entries along the main diagonal
	float Matrix4::trace() const
	{
		const float trace = data[0][0] + data[1][1] + data[2][2] + data[3][3];
		return trace;
	}
	
	// Determinant:
	// Takes and returns the determinant of the
	// matrix.
	float Matrix4::determinant() const
	{
		float det = 0;

		// Sum the cofactors of the first column
		for (int row = 0; row < 4; row++)
			det += data[0][row] * cofactor(0, row);

		return det;
	}

	// Minor:
	// Returns the minor of the matrix for a
	// given row and col
	float Matrix4::minor(int col, int row) const
	{
		// Create a 3x3 matrix by deleting the row
		// and col of this matrix
		Matrix3 matrix3;

		// Counter to iterate through our 3x3 matrix
		int index = 0;

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				// Only add entries to matrix whose row and 
				// col don't match
				if (i != col && j != row)
				{
					// Add to 3x3 matrix
					matrix3[index / 3][index % 3] = data[i][j];

					index++;
				}

			}
		}

		// The minor is the determinant of this matrix
		return matrix3.determinant();
	}

	// Cofactor:
	// Returns the cofactor of the matrix for a
	// given row and col.
	float Matrix4::cofactor(int col, int row) const
	{
		const int sign = (row + col) % 2 == 0 ? 1 : -1;
		return sign * minor(col, row);
	}
	

	/* --- Operands --- */
	// Index:
	// Access a given element of the matrix by row
	// and col indices
	float* const Matrix4::operator[](int col)
	{
		return data[col];
	}

	// Index (Constant Version):
	// Access a given element of the matrix by row
	// and col indices
	const float* const Matrix4::operator[](int col) const
	{
		return data[col];
	}

	// Multiply (Matrix):
	// Multiplies two Matrix4's together
	Matrix4 Matrix4::operator*(const Matrix4& matrix) const
	{
		Matrix4 new_matrix = Matrix4();

		for (int col = 0; col < 4; col++)
		{
			for (int row = 0; row < 4; row++)
			{
				float value = 0;

				for (int i = 0; i < 4; i++)
					value += data[i][row] * matrix[col][i];

				new_matrix[col][row] = value;
			}
		}

		return new_matrix;
	}

	// Multiply (Vector):
	// Multiplies a Matrix4 with a Vector4
	// and returns the result
	Vector4 Matrix4::operator*(const Vector4& vec) const
	{
		const float x = data[0][0] * vec.x + data[1][0] * vec.y + data[2][0] * vec.z + data[3][0] * vec.w;
		const float y = data[0][1] * vec.x + data[1][1] * vec.y + data[2][1] * vec.z + data[3][1] * vec.w;
		const float z = data[0][2] * vec.x + data[1][2] * vec.y + data[2][2] * vec.z + data[3][2] * vec.w;
		const float w = data[0][3] * vec.x + data[1][3] * vec.y + data[2][3] * vec.z + data[3][3] * vec.w;

		return Vector4(x, y, z, w);
	}

	// Multiply (Scalar):
	// Multiplies a Matrix4 with a float
	// and returns the result
	Matrix4 Matrix4::operator*(const float c) const
	{
		Matrix4 new_matrix;

		for (int col = 0; col < 4; col++)
			for (int row = 0; row < 4; row++)
				new_matrix[col][row] = data[col][row] * c;

		return new_matrix;
	}

	// Divide (Scalar):
	// Divides a Matrix4 by a float
	// and returns the result
	Matrix4 Matrix4::operator/(const float c) const
	{
		Matrix4 new_matrix;

		for (int col = 0; col < 4; col++)
			for (int row = 0; row < 4; row++)
				new_matrix[col][row] = data[col][row] / c;

		return new_matrix;
	}

} // Namespace Math
} // Namespace Engine