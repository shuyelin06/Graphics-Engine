#pragma once

#include "math/Triangle.h"

namespace Engine {
using namespace Math;
namespace Datamodel {
// MarchingCube Class:
// Represents a cube with float values at each of its 8 vertices. These float
// values can be positive or negative, and we assume that a surface exists where
// these values are 0 (after linearly interpolating the vertex values across the
// entire cube). This class implements the marching cubes algorithm to generate
// a non-ambiguous triangulation for a surface approximating where these values
// are 0.
class MarchingCube {
  private:
    // Data is given in order of the vertex id mapping shown above
    float vertexData[8];

    // Output fields
    Triangle* output_triangulation;
    int* output_num_triangles;

  public:
    MarchingCube();
    ~MarchingCube();

    // Set the 8 vertices of the MarchingCube
    void updateData(float a1, float a2, float a3, float a4, float a5, float a6,
                    float a7, float a8);
    // Stream the triangulation of the data to the parameter output.
    // Expected that this output consists of 12 triangles or more.
    void generateSurface(Triangle* triangle_output, int* num_triangles);

  private:
    void createTriangles(const char* edgeList, char numberTriangles);

    Vector3 generateVertexOnEdge(char edgeID);

    bool testFaceAmbiguity(char faceID);
    bool testInternalAmbiguity(char caseID, char configID, char subConfigID,
                               char sign);

    char computeVertexMask();
};
} // namespace Datamodel
} // namespace Engine