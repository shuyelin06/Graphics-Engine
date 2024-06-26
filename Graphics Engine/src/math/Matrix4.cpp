#include "Matrix4.h"
#include "Matrix3.h"

namespace Engine
{
namespace Math
{
	/* --- Constructors --- */
	// Default Constructor: 
	// Initialize a 4x4 matrix with only 0s
	// as entries
	Matrix4::Matrix4() :
		data{ {0} } {}

	// Constructor:
	// Initialize a 4x4 matrix with any 16 values as
	// entries. Initialization occurs in order of
	// column, then row (a11, a12, a13, a14, a21...).
	Matrix4::Matrix4(float c1, float c2, float c3, float c4,
					float c5, float c6, float c7, float c8,
					float c9, float c10, float c11, float c12,
					float c13, float c14, float c15, float c16) 
		: data{ {c1, c2, c3, c4}, 
				{c5, c6, c7, c8}, 
				{c9, c10, c11, c12}, 
				{c13, c14, c15, c16} } {}

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

		for (int row = 0; row < 4; row++)
		{
			for (int col = 0; col < 4; col++)
			{
				matrix_transpose[row][col] = data[col][row];
			}
		}

		return matrix_transpose;
	}

	// Inverse:
	// Takes and returns the inverse of a matrix,
	// using the adjugate method
	Matrix4 Matrix4::inverse() const
	{
		Matrix4 matrix_inverse;

		// Find determinant
		float det = determinant();

		// Build inverse matrix by simultaneously taking
		// the transpose of the cofactor matrix
		// and dividing it by the determinant
		for (int row = 0; row < 4; row++)
		{
			for (int col = 0; col < 4; col++)
			{
				matrix_inverse[row][col] = cofactor(col, row) / det;
			}
		}

		return matrix_inverse;
	}

	// Trace:
	// Takes the trace of the matrix, the sum of the
	// entries along the main diagonal
	float Matrix4::trace() const
	{
		float sum = 0;

		for (int i = 0; i < 4; i++)
		{
			sum += data[i][i];
		}

		return sum;
	}
	
	// Determinant:
	// Takes and returns the determinant of the
	// matrix.
	float Matrix4::determinant() const
	{
		float det = 0;

		// Sum the cofactors of the first row
		for (int col = 0; col < 4; col++)
		{
			det += data[0][col] * cofactor(0, col);
		}

		return det;
	}

	// Minor:
	// Returns the minor of the matrix for a
	// given row and col
	float Matrix4::minor(int row, int col) const
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
				if (i != row && j != col)
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
	float Matrix4::cofactor(int row, int col) const
	{
		int sign = (row + col) % 2 == 0 ? 1 : -1;
		return minor(row, col) * sign;
	}
	

	/* --- Operands --- */
	// Index:
	// Access a given element of the matrix by row
	// and col indices
	float* const Matrix4::operator[](int row)
	{
		return data[row];
	}

	// Index (Constant Version):
	// Access a given element of the matrix by row
	// and col indices
	const float* const Matrix4::operator[](int row) const
	{
		return data[row];
	}

	// Multiply (Matrix):
	// Multiplies two Matrix4's together
	Matrix4 Matrix4::operator*(const Matrix4& matrix) const
	{
		Matrix4 new_matrix = Matrix4();

		for (int row = 0; row < 4; row++)
		{
			for (int col = 0; col < 4; col++)
			{
				float value = 0;

				for (int i = 0; i < 4; i++)
				{
					value += data[row][i] * matrix[i][col];
				}

				new_matrix[row][col] = value;
			}
		}

		return new_matrix;
	}

	// Multiply (Vector):
	// Multiplies a Matrix4 with a Vector4
	// and returns the result
	Vector4 Matrix4::operator*(const Vector4& vec) const
	{
		float x = data[0][0] * vec.x + data[0][1] * vec.y + data[0][2] * vec.z + data[0][3] * vec.w;
		float y = data[1][0] * vec.x + data[1][1] * vec.y + data[1][2] * vec.z + data[1][3] * vec.w;
		float z = data[2][0] * vec.x + data[2][1] * vec.y + data[2][2] * vec.z + data[2][3] * vec.w;
		float w = data[3][0] * vec.x + data[3][1] * vec.y + data[3][2] * vec.z + data[3][3] * vec.w;

		return Vector4(x, y, z, w);
	}

	// Multiply (Scalar):
	// Multiplies a Matrix4 with a float
	// and returns the result
	Matrix4 Matrix4::operator*(const float c) const
	{
		Matrix4 new_matrix;

		for (int row = 0; row < 4; row++)
		{
			for (int col = 0; col < 4; col++)
			{
				new_matrix[row][col] = data[row][col] * c;
			}
		}

		return new_matrix;
	}

	// Divide (Scalar):
	// Divides a Matrix4 by a float
	// and returns the result
	Matrix4 Matrix4::operator/(const float c) const
	{
		Matrix4 new_matrix;

		for (int row = 0; row < 4; row++)
		{
			for (int col = 0; col < 4; col++)
			{
				new_matrix[row][col] = data[row][col] / c;
			}
		}

		return new_matrix;
	}

} // Namespace Math
} // Namespace Engine