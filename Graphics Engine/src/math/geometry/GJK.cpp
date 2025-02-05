#include "GJK.h"

#include <assert.h>
#include <math.h>

#include "GJKSupport.h"
#include "QuickHull.h"
#include "../Matrix4.h"
#include "../Quaternion.h"

#include "rendering/VisualDebug.h"

namespace Engine {
namespace Math {
GJKSolver::GJKSolver(GJKSupportFunc* _shape_1, GJKSupportFunc* _shape_2) {
    shape_1 = _shape_1;
    shape_2 = _shape_2;

    simplex = nullptr;
}

// CheckIntersection:
// Returns true if the shapes intersect, false if they do not.
// To do this, GJK is checking if the Minkowski Difference between the
// two shapes contains the origin.
// To do this, it attempts to "smartly" choose directions to create a simplex
// within this difference, and checks if this simplex contains the origin or
// not.
struct GJKSimplex {
    Vector3 points[4];
    int num_points;

    GJKSimplex() : points() { num_points = 0; }

    void swap(int i1, int i2) {
        const Vector3 temp = points[i1];
        points[i1] = points[i2];
        points[i2] = temp;
    }
    void push_back(const Vector3& p) { points[num_points++] = p; }
    void remove(int index) {
        for (int i = index; i < num_points - 1; i++)
            points[index] = points[index + 1];
        num_points--;
    }

    // Last (p1) to first (p4) vertex inserted, in that order
    const Vector3& p1() const { return points[num_points - 1]; }
    const Vector3& p2() const { return points[num_points - 2]; }
    const Vector3& p3() const { return points[num_points - 3]; }
    const Vector3& p4() const { return points[num_points - 4]; }
};

// CheckIntersection:
// Returns whether or not the shapes intersect.
// True on intersection, false if none.
bool GJKSolver::checkIntersection() {
    if (simplex != nullptr)
        delete simplex;
    simplex = new GJKSimplex();

    SolverStatus status = Evolving;
    while (status == Evolving)
        status = iterate();

    return status == IntersectionTrue;
}

// Iterate:
// Represents one iteration of the GJK algorithm.
// The behavior changes based on the number of points we have in the
// simplex already.
SolverStatus GJKSolver::iterate() {
    // Attempt to grow our simplex. We do this by selecting a "good"
    // direction to query our support functions, depending on how many
    // points we currently have in the simplex. When our simplex is full, we
    // start checking if it contains our origin point.
    switch (simplex->num_points) {
    // Empty Simplex: Choose some initial direction.
    // Direction can be whatever we want. Commonly, it is the
    // direction pointing from one shape center to the other.
    case 0: {
        direction = shape_1->center() - shape_2->center();
    } break;

    // Single Point:
    // Flip the direction
    case 1: {
        direction = -direction;
    } break;

    // Line:
    // Direction is the vector orthogonal to the line p1, p2,
    // pointing towards the origin.
    case 2: {
        const Vector3& A = simplex->p1();
        const Vector3& B = simplex->p2();

        const Vector3 AB = B - A;
        const Vector3 AO = -A;

        direction = (AB.cross(AO)).cross(AB);
    } break;

    // Triangle:
    // Direction is the normal of the triangle pointing towards the
    // origin.
    case 3: {
        const Vector3& A = simplex->p1();
        const Vector3& B = simplex->p2();
        const Vector3& C = simplex->p3();

        // Calculate the edges of my triangle and find the normal
        const Vector3 AC = C - A;
        const Vector3 AB = B - A;
        direction = AC.cross(AB);

        // Flip normal if it is not pointing towards the origin.
        const Vector3 AO = -A;
        if (direction.dot(AO) < 0) {
            direction = -direction;

            // Flip orientation of triangle if it is facing the wrong way.
            // This way, when we generate our tetrahedron, the face normals will
            // correctly point outwards.
            simplex->swap(1, 2);
        }

    } break;

    // Tetrahedron:
    // We have a full simplex. We now check to see where the
    // origin could be.
    case 4: {
        const Vector3& A = simplex->p1();
        const Vector3& B = simplex->p2();
        const Vector3& C = simplex->p3();
        const Vector3& D = simplex->p4();

        // Calculate edges of the tetrahedron. We only care about the
        // edges from A to every other vertex.
        const Vector3 AB = B - A;
        const Vector3 AC = C - A;
        const Vector3 AD = D - A;

        // Find direction of A to the origin
        const Vector3 AO = -A;

        // We find the norms of each of the tetrahedron's sides, and
        // compare it with the direction to the origin to see
        // where the origin lies.
        // If the dot between AO and the norm is positive, then
        // the 4th point not included in the triangle is not on the
        // side of the origin. We can thus remove that point.
        const Vector3 ABCNorm = AC.cross(AB);
        const Vector3 ACDNorm = AD.cross(AC);
        const Vector3 ADBNorm = AB.cross(AD);

        // If we know what face the origin is outside, we will correct our
        // simplex so that the triangle is clock-wise when viewed from the
        // origin (so that our algorithm chooses the correct direction later).
        if (ABCNorm.dot(AO) > 0.0f) {
            simplex->remove(0); // Remove Point D
            direction = ABCNorm;
        } else if (ACDNorm.dot(AO) > 0.0f) {
            simplex->remove(2); // Remove Point B
            direction = ACDNorm;
        } else if (ADBNorm.dot(AO) > 0.0f) {
            simplex->remove(1); // Remove Point C
            direction = ADBNorm;
        }
        // If not outside any of the triangles, then origin
        // is within the tetrahedron!
        else {
            return IntersectionTrue;
        }
    } break;
    }

    // With our direction, we query to find our support point.
    // If the new vertex.dot(direction) is < 0, then origin cannot
    // exist inside our Minkowski Difference.
    const Vector3 newVertex = querySupports(direction);
    if (direction.dot(newVertex) < 0.0f) {
        return IntersectionFalse;
    } else {
        simplex->push_back(newVertex);
        return Evolving;
    }
}

// PenetrationVector:
// If an intersection is found, returns the vector of penetration between
// the two shapes. Does this using the expanding prototype algorithm (EPA)
// https://allenchou.net/2013/12/game-physics-contact-generation-epa/
Vector3 GJKSolver::penetrationVector() {
    Vector3 penetration = Vector3::VectorMax();
    float distance = FLT_MAX;

    const int SAMPLES_THETA = 15;
    const int SAMPLES_PHI = 10;

    for (int i = 0; i < SAMPLES_THETA; i++) {
        const float theta = i * (2 * 3.14159f) / SAMPLES_THETA;

        for (int j = 0; j < SAMPLES_PHI; j++) {
            const float phi = j * (3.14159) / SAMPLES_PHI;

            const Quaternion rotation = Quaternion::RotationAroundAxis(Vector3::PositiveZ(), theta) * Quaternion::RotationAroundAxis(Vector3::PositiveY(), phi);
            const Matrix3 m_rotation = rotation.rotationMatrix3();
            const Vector3 direction = m_rotation * Vector3::PositiveZ();

            Graphics::VisualDebug::DrawLine(Vector3(), direction * 3, Color::White());

            const Vector3 support_point = querySupports(direction);
            Graphics::VisualDebug::DrawPoint(support_point, 1.25f);

            if (support_point.dot(direction) < distance) {
                penetration = -direction * support_point.dot(direction);
                distance = support_point.dot(direction);
            }
        }
    }

    return penetration;
}

// QuerySupports:
// Given a direction, queries the suport functions to find the corresponding
// support point in the Minkowski Difference
const Vector3 GJKSolver::querySupports(const Vector3& direction) {
    return shape_1->furthestPoint(direction) -
           shape_2->furthestPoint(-direction);
}

} // namespace Math
} // namespace Engine