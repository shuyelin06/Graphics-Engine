#include "GJK.h"

#include <math.h>

#include "GJKSupport.h"

namespace Engine {
namespace Math {
GJKSolver::GJKSolver(GJKSupportFunc* _shape_1, GJKSupportFunc* _shape_2) {
    shape_1 = _shape_1;
    shape_2 = _shape_2;
}

// CheckIntersection:
// Returns true if the shapes intersect, false if they do not.
// To do this, GJK is checking if the Minkowski Difference between the
// two shapes contains the origin.
// To do this, it attempts to "smartly" choose directions to create a simplex
// within this difference, and checks if this simplex contains the origin or
// not.
static constexpr float EPSILON = 0.001f;

GJKSimplex::GJKSimplex() : points() { num_points = 0; }
void GJKSimplex::push_back(const Vector3& p) { points[num_points++] = p; }
int GJKSimplex::size() const { return num_points; }
void GJKSimplex::remove(int index) {
    for (int i = index; i < 3; i++) {
        points[i] = points[i + 1];
    }

    num_points--;
}

const Vector3 GJKSimplex::p1() const { return points[num_points - 1]; }
const Vector3 GJKSimplex::p2() const { return points[num_points - 2]; }
const Vector3 GJKSimplex::p3() const { return points[num_points - 3]; }
const Vector3 GJKSimplex::p4() const { return points[num_points - 4]; }

// CheckIntersection:
// Returns whether or not the shapes intersect.
// True on intersection, false if none.
bool GJKSolver::checkIntersection() {
    simplex = GJKSimplex();

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
    switch (simplex.size()) {
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
        const Vector3 A = simplex.p1();
        const Vector3 B = simplex.p2();

        const Vector3 AB = B - A;
        const Vector3 AO = -A;

        direction = (AB.cross(AO)).cross(AB);
    } break;

    // Triangle:
    // Direction is the normal of the triangle pointing towards the
    // origin.
    case 3: {
        const Vector3 A = simplex.p1();
        const Vector3 B = simplex.p2();
        const Vector3 C = simplex.p3();

        // Calculate the edges of my triangle and find the normal
        const Vector3 AC = C - A;
        const Vector3 AB = B - A;
        direction = AC.cross(AB);

        // Flip normal if it is not pointing towards the origin.
        const Vector3 AO = -A;
        if (direction.dot(AO) < 0) {
            direction = -direction;
             
            // Flip orientation of triangle
            const Vector3 temp = simplex.points[1];
            simplex.points[1] = simplex.points[2];
            simplex.points[2] = temp;
        }
            

    } break;

    // Tetrahedron:
    // We have a full simplex. We now check to see where the
    // origin could be.
    case 4: {
        const Vector3 A = simplex.p1();
        const Vector3 B = simplex.p2();
        const Vector3 C = simplex.p3();
        const Vector3 D = simplex.p4();

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

        constexpr int A_INDEX = 3;
        constexpr int B_INDEX = 2;
        constexpr int C_INDEX = 1;
        constexpr int D_INDEX = 0;

        if (ABCNorm.dot(AO) > EPSILON) {
            simplex.num_points = 3;
            simplex.points[2] = A;
            simplex.points[1] = B;
            simplex.points[0] = C;
            direction = ABCNorm;
        } else if (ACDNorm.dot(AO) > EPSILON) {
            simplex.num_points = 3;
            simplex.points[2] = A;
            simplex.points[1] = C;
            simplex.points[0] = D;
            direction = ACDNorm;
        } else if (ADBNorm.dot(AO) > EPSILON) {
            simplex.num_points = 3;
            simplex.points[2] = A;
            simplex.points[1] = D;
            simplex.points[0] = B;
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
    if (direction.dot(newVertex) < EPSILON) {
        return IntersectionFalse;
    } else {
        simplex.push_back(newVertex);
        return Evolving;
    }
}

// PenetrationVector:
// If an intersection is found, returns the vector of penetration between
// the two shapes.
// Does this by sampling directions on the Minkowski difference and choosing
// the one with the smallest distance.
Vector3 GJKSolver::penetrationVector() {
    Vector3 directions[14] = {Vector3(1, 0, 0),
                              Vector3(0, 1, 0),
                              Vector3(0, 0, 1),
                              Vector3(-1, 0, 0),
                              Vector3(0, -1, 0),
                              Vector3(0, 0, -1),
                              Vector3(0.5773f, 0.5773f, 0.5773f),
                              Vector3(0.5773f, 0.5773f, -0.5773f),
                              Vector3(0.5773f, -0.5773f, 0.5773f),
                              Vector3(0.5773f, -0.5773f, -0.5773f),
                              Vector3(-0.5773f, 0.5773f, 0.5773f),
                              Vector3(-0.5773f, 0.5773f, -0.5773f),
                              Vector3(-0.5773f, -0.5773f, 0.5773f),
                              Vector3(-0.5773f, -0.5773f, -0.5773f)};

    Vector3 penetration = Vector3::VectorMax();
    float distance = 100000;

    for (const Vector3& direction : directions) {
        const Vector3 support_point = querySupports(direction);
        if (support_point.dot(direction) <= distance) {
            penetration = support_point;
            distance = support_point.dot(direction);
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