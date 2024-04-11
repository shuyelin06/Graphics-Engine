#pragma once

namespace Engine
{
namespace Math
{

	// Vector3
	// Contains methods and data for a 3-dimensional
	// vector.
	class Vector3
	{
	public:
		float x, y, z;

		Vector3();
		Vector3(const Vector3& copy);
		Vector3(float _x, float _y, float _z);

		~Vector3() {};

		// In-place normalize
		void inplaceNormalize(); 

		// Returns the vector's magnitude
		float magnitude() const;

		// Vector Operations
		Vector3 operator+(const Vector3&) const; // Addition
		Vector3& operator+=(const Vector3&);	 // Compound (In-Place) Addition
		Vector3 operator-(const Vector3&) const; // Subtraction
		Vector3& operator-=(const Vector3&);	 // Compound (In-Place) Subtraction 
		Vector3 operator-() const;				 // Negation
		
		Vector3 operator*(const float) const;    // Scalar Multiplication
		Vector3& operator*=(const float);		 // Compound (In-Place) Scalar Multiplication	
		Vector3 operator/(const float) const;	 // Scalar Division
		Vector3& operator/=(const float);		 // Compound (In-Place) Scalar Division

		// Static Vector Operations
		static Vector3 CrossProduct(Vector3 v1, Vector3 v2);
		static float DotProduct(Vector3 v1, Vector3 v2);

		static Vector3 PositiveX();
		static Vector3 PositiveY();
		static Vector3 PositiveZ();
	};


} // Namespace Math
} // Namespace Engine