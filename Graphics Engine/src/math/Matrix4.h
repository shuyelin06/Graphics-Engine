#pragma once

#include "Vector4.h"

namespace Engine
{
namespace Math
{

	// Matrix4
	// Contains methods and data for a 4x4
	// matrix.
	class Matrix4
	{
	protected:
		float data[4][4];

	public:
		Matrix4(); // Initialize to 0
		Matrix4(float, float, float, float,
				float, float, float, float,
				float, float, float, float,
				float, float, float, float);

		Matrix4 tranpose() const;
		Matrix4 inverse() const;
		
		float trace() const;
		float determinant() const; 

		float minor(int row, int col) const;
		float cofactor(int row, int col) const;

		float* const operator[](int); // Access an element by index

		Matrix4 operator*(Matrix4&) const;
		Vector4 operator*(const Vector4&) const;
		Matrix4 operator*(const float) const;
		Matrix4 operator/(const float) const;
	};

} // Namespace Math
} // Namespace Engine
