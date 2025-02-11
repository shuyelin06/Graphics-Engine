#pragma once

#include "ConvexHull.h"
#include "Triangle.h"

namespace Engine {
namespace Math {
struct QuickHullData;

// QuickHullSolver Class:
// Implements the 3D QuickHull algorithm for convex hull generation. Saves its
// state internally so that the algorithm can be used to incrementally build
// hulls. Hides its implementation in the .cpp source file.
class QuickHullSolver {
  private:
    QuickHullData* solver_data;

  public:
    QuickHullSolver();
    ~QuickHullSolver();

    // Creates an instance of a Convex Hull given the loaded point data.
    ConvexHull* getHull() const;

    // Loads the point data and runs the QuickHull algorithm.
    void computeConvexHull(const std::vector<Vector3>& point_cloud);
    void addPointToHull(const Vector3& point);
    
    // Returns the triangle representing the face closest to the origin. Used in
    // EPA (see GJK.cpp)
    Triangle closestFaceToOrigin(float* distance_out) const;

  private:
    // Helper methods for QuickHull
    void generateInitialHull(const std::vector<Vector3>& point_cloud);
    int reassignPointsToFaces();
    void findHorizonEdge(int point, int face, int prev_face);
};

} // namespace Math
} // namespace Engine