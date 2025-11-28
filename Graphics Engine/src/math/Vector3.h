#pragma once

#include <functional>

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

    ~Vector3() {};

    // Updates the contents of the current vector
    void set(const Vector3& vec);

    // Return a vector with the contents rearranged
    Vector3 xzy() const;

    // Normalize the vector
    void inplaceNormalize(); // In-Place Normalize
    Vector3 unit() const;    // Returns a Normalized Copy

    float magnitude() const;

    // Dot and Cross Product
    float dot(const Vector3& vector) const;
    Vector3 cross(const Vector3& vector) const;

    // Projects this vector onto another vector.
    float scalarProjection(const Vector3& vector) const;
    Vector3 projectOnto(const Vector3& vector) const;

    // Returns a new vector with each x,y,z value the minimum (or maximum) of
    // the two vectors
    Vector3 componentMin(const Vector3& vector) const;
    Vector3 componentMax(const Vector3& vector) const;

    // Returns a new vector that is orthogonal to this vector
    Vector3 orthogonal() const;

    // Vector Operations
    float operator[](int) const;

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
    Vector3&
    operator*=(const Vector3&); // Compound Hamming (Component-Wise) Product

    bool operator==(const Vector3& vec) const; // Equality -- Used in Hashing

    // Static Vector Operations
    static Vector3 PositiveX();
    static Vector3 PositiveY();
    static Vector3 PositiveZ();
    static Vector3 NegativeX();
    static Vector3 NegativeY();
    static Vector3 NegativeZ();

    static Vector3 VectorMax();
    static Vector3 VectorMin();

    static Vector3 Lerp(const Vector3& a, const Vector3& b, float time);
};

} // Namespace Math
} // Namespace Engine

// Hash Function for Vector3
template <> struct std::hash<Engine::Math::Vector3> {
    std::size_t operator()(const Engine::Math::Vector3& k) const {
        // https://stackoverflow.com/questions/5928725/hashing-2d-3d-and-nd-vectors
        uint32_t hash = std::_Bit_cast<uint32_t, float>(k.x) * 73856093 ^
                        std::_Bit_cast<uint32_t, float>(k.y) * 19349663 ^
                        std::_Bit_cast<uint32_t, float>(k.z) * 83492791;

        return hash % SIZE_MAX;
    }
};