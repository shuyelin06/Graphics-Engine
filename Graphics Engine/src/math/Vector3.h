#pragma once

namespace Engine {
namespace Math {

// Vector3
// Contains methods and data for a 3-dimensional
// vector.
class Vector3 {
  public:
    float x, y, z;

    Vector3();
    Vector3(const Vector3& copy);
    Vector3(float _x, float _y, float _z);

    ~Vector3(){};

    // Updates the contents of the current vector
    void set(const Vector3& vec); 

    // Normalize the vector
    void inplaceNormalize(); // In-Place Normalize
    Vector3 unit() const;    // Returns a Normalized Copy

    float magnitude() const;

    // Dot and Cross Product
    float dot(const Vector3& vector) const;
    Vector3 cross(const Vector3& vector) const;
    
    // Projects this vector onto another vector.
    Vector3 projectOnto(const Vector3& vector) const;

    // Returns a new vector with each x,y,z value the minimum (or maximum) of the two vectors
    Vector3 componentMin(const Vector3& vector) const;
    Vector3 componentMax(const Vector3& vector) const;

    // Vector Operations
    Vector3 operator+(const Vector3&) const; // Addition
    Vector3& operator+=(const Vector3&);     // Compound (In-Place) Addition
    Vector3 operator-(const Vector3&) const; // Subtraction
    Vector3& operator-=(const Vector3&);     // Compound (In-Place) Subtraction
    Vector3 operator-() const;               // Negation

    Vector3 operator*(const float) const; // Scalar Multiplication
    Vector3&
    operator*=(const float); // Compound (In-Place) Scalar Multiplication
    Vector3 operator/(const float) const; // Scalar Division
    Vector3& operator/=(const float);     // Compound (In-Place) Scalar Division

    Vector3 operator*(const Vector3&) const; // Hamming (Component-Wise) Product
    Vector3& operator*=(const Vector3&);    // Compound Hamming (Component-Wise) Product

    // Static Vector Operations
    static Vector3 PositiveX();
    static Vector3 PositiveY();
    static Vector3 PositiveZ();
    static Vector3 NegativeX();
    static Vector3 NegativeY();
    static Vector3 NegativeZ();

    static Vector3 VectorMax();
    static Vector3 VectorMin();
};

} // Namespace Math
} // Namespace Engine