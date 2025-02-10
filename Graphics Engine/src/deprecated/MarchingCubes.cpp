#include <assert.h>

#include <cmath>
#include <math.h>

#include "MarchingCubeTables.h"
#include "math/Compute.h"
#include "math/Matrix3.h"
#include "math/Triangle.h"

constexpr float FLT_EPSILON = 0.0001f;

namespace Engine {
using namespace Math;
namespace Deprecated {

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

    void updateData(float a1, float a2, float a3, float a4, float a5, float a6,
                    float a7, float a8);
    void generateSurface(Triangle* triangle_output, int* num_triangles);

  private:
    void createTriangles(const char* edgeList, char numberTriangles);

    Vector3 generateVertexOnEdge(char edgeID);

    bool testFaceAmbiguity(char faceID);
    bool testInternalAmbiguity(char caseID, char configID, char subConfigID,
                               char sign);

    char computeVertexMask();
};

// GenerateTerrainAsset:
// For a given terrain chunk, we will generate the mesh for it using Marching
// Cubes.
/*
Mesh* GenerateTerrainMesh(MeshBuilder& builder,
                                        TerrainData data) {
    builder.reset();

    MarchingCube marchingCube;
    Triangle triangulation[10];
    int num_triangles;

    for (int i = 0; i < TERRAIN_CHUNK_X_SAMPLES - 1; i++) {
        for (int j = 0; j < TERRAIN_CHUNK_Y_SAMPLES - 1; j++) {
            for (int k = 0; k < TERRAIN_CHUNK_Z_SAMPLES - 1; k++) {
                // Load data into a marching cube
                marchingCube.updateData(
                    data.sample(i, j, k), data.sample(i + 1, j, k),
                    data.sample(i + 1, j + 1, k), data.sample(i, j + 1, k),
                    data.sample(i, j, k + 1), data.sample(i + 1, j, k + 1),
                    data.sample(i + 1, j + 1, k + 1),
                    data.sample(i, j + 1, k + 1));

                // Generate triangulation for this cube
                marchingCube.generateSurface(triangulation, &num_triangles);

                // Scale and transform to voxel location
                const float x_scale =
                    TERRAIN_SIZE / (TERRAIN_CHUNK_X_SAMPLES - 1);
                const float z_scale =
                    TERRAIN_SIZE / (TERRAIN_CHUNK_Z_SAMPLES - 1);
                const float y_scale = TERRAIN_HEIGHT / TERRAIN_CHUNK_Y_SAMPLES;

                Vector3 offset = Vector3(i, j, k);
                Vector3 scale = Vector3(x_scale, y_scale, z_scale);

                for (int tri_index = 0; tri_index < num_triangles;
                     tri_index++) {
                    const Triangle& triangle = triangulation[tri_index];

                    MeshVertex meshVertex;

                    // Flip orientation of triangle
                    UINT v0 =
                        builder.addVertex((triangle.vertex(0) + offset) * scale,
                                          Vector2(), Vector3());
                    UINT v1 =
                        builder.addVertex((triangle.vertex(2) + offset) * scale,
                                          Vector2(), Vector3());
                    UINT v2 =
                        builder.addVertex((triangle.vertex(1) + offset) * scale,
                                          Vector2(), Vector3());

                    builder.addTriangle(v0, v1, v2);
                }
            }
        }
    }

    builder.regenerateNormals();

    return builder.generate();
}
*/

// -------------------------------------------------------------
// Terrain Generation using an adapted version of Lewiner, et. al's
// 3D Marching Cubes implementation.
// http://thomas.lewiner.org/pdfs/marching_cubes_jgt.pdf
/*
 * Cube Mappings:
 * X-axis goes left to right, Z-axis goes bottom to top, Y-axis goes back to
 * front.
 *
 *         7 ________ 6           _____6__             ________
 *         /|       /|         7/|       /|          /|       /|
 *       /  |     /  |        /  |     /5 |        /  6     /  |
 *   4 /_______ /    |      /__4____ /    10     /_______3/    |
 *    |     |  |5    |     |    11  |     |     |     |  |   2 |
 *    |    3|__|_____|2    |     |__|__2__|     | 4   |__|_____|
 *    |    /   |    /      8   3/   9    /      |    /   |    /
 *    |  /     |  /        |  /     |  /1       |  /     5  /
 *    |/_______|/          |/___0___|/          |/_1_____|/
 *   0          1
 *
 * In order from left to right:
 * 1) Vertex IDs
 * 2) Edge IDs
 * 3) Face IDs
 */

MarchingCube::MarchingCube() = default;
MarchingCube::~MarchingCube() = default;

void MarchingCube::updateData(float a1, float a2, float a3, float a4, float a5,
                              float a6, float a7, float a8) {
    vertexData[0] = a1;
    vertexData[1] = a2;
    vertexData[2] = a3;
    vertexData[3] = a4;
    vertexData[4] = a5;
    vertexData[5] = a6;
    vertexData[6] = a7;
    vertexData[7] = a8;
}

// Generates a surface for the marching cube. Assumes that data is loaded into
// vertexData. This uses the Marching Cube algorithm, and assumes the surface
// exists at the value 0.
void MarchingCube::generateSurface(Triangle* triangle_output,
                                   int* num_triangles) {
    assert(triangle_output != nullptr && num_triangles != nullptr);

    output_triangulation = triangle_output;
    output_num_triangles = num_triangles;
    (*output_num_triangles) = 0;

    const unsigned char vertexMask = computeVertexMask();

    const char caseID = CaseTable[vertexMask][0];
    const char configID = CaseTable[vertexMask][1];
    char subconfigID = 0;

    switch (caseID) {
    case 0:
        break;

    case 1:
        createTriangles(TilingTableCase1[configID], 1);
        break;

    case 2:
        createTriangles(TilingTableCase2[configID], 2);
        break;

    case 3:
        if (testFaceAmbiguity(TestTableCase3[configID]))
            createTriangles(TilingTableCase3_2[configID], 4); // 3.2
        else
            createTriangles(TilingTableCase3_1[configID], 2); // 3.1
        break;

    case 4:
        if (testInternalAmbiguity(caseID, configID, subconfigID,
                                  TestTableCase4[configID]))
            createTriangles(TilingTableCase4_1[configID], 2); // 4.1.1
        else
            createTriangles(TilingTableCase4_2[configID], 6); // 4.1.2
        break;

    case 5:
        createTriangles(TilingTableCase5[configID], 3);
        break;

    case 6:
        if (testFaceAmbiguity(TestTableCase6[configID][0]))
            createTriangles(TilingTableCase6_2[configID], 5); // 6.2
        else {
            if (testInternalAmbiguity(caseID, configID, subconfigID,
                                      TestTableCase6[configID][1]))
                createTriangles(TilingTableCase6_1_1[configID], 3); // 6.1.1
            else {
                createTriangles(TilingTableCase6_1_2[configID], 9); // 6.1.2
            }
        }
        break;

    case 7:
        if (testFaceAmbiguity(TestTableCase7[configID][0]))
            subconfigID += 1;
        if (testFaceAmbiguity(TestTableCase7[configID][1]))
            subconfigID += 2;
        if (testFaceAmbiguity(TestTableCase7[configID][2]))
            subconfigID += 4;
        switch (subconfigID) {
        case 0:
            createTriangles(TilingTableCase7_1[configID], 3);
            break;
        case 1:
            createTriangles(TilingTableCase7_2[configID][0], 5);
            break;
        case 2:
            createTriangles(TilingTableCase7_2[configID][1], 5);
            break;
        case 3:
            createTriangles(TilingTableCase7_3[configID][0], 9);
            break;
        case 4:
            createTriangles(TilingTableCase7_2[configID][2], 5);
            break;
        case 5:
            createTriangles(TilingTableCase7_3[configID][1], 9);
            break;
        case 6:
            createTriangles(TilingTableCase7_3[configID][2], 9);
            break;
        case 7:
            if (testInternalAmbiguity(caseID, configID, subconfigID,
                                      TestTableCase7[configID][3]))
                createTriangles(TilingTableCase7_4_2[configID], 9);
            else
                createTriangles(TilingTableCase7_4_1[configID], 5);
            break;
        };
        break;

    case 8:
        createTriangles(TilingTableCase8[configID], 2);
        break;

    case 9:
        createTriangles(TilingTableCase9[configID], 4);
        break;

    case 10:
        if (testFaceAmbiguity(TestTableCase10[configID][0])) {
            if (testFaceAmbiguity(TestTableCase10[configID][1]))
                createTriangles(TilingTableCase10_1_1_Inverted[configID],
                                4); // 10.1.1
            else {
                createTriangles(TilingTableCase10_2[configID], 8); // 10.2
            }
        } else {
            if (testFaceAmbiguity(TestTableCase10[configID][1])) {
                createTriangles(TilingTableCase10_2_Inverted[configID],
                                8); // 10.2
            } else {
                if (testInternalAmbiguity(caseID, configID, subconfigID,
                                          TestTableCase10[configID][2]))
                    createTriangles(TilingTableCase10_1_1[configID],
                                    4); // 10.1.1
                else
                    createTriangles(TilingTableCase10_1_2[configID],
                                    8); // 10.1.2
            }
        }
        break;

    case 11:
        createTriangles(TilingTableCase11[configID], 4);
        break;

    case 12:
        if (testFaceAmbiguity(TestTableCase12[configID][0])) {
            if (testFaceAmbiguity(TestTableCase12[configID][1]))
                createTriangles(TilingTableCase12_1_1_Inverted[configID],
                                4); // 12.1.1
            else {
                createTriangles(TilingTableCase12_2[configID], 8); // 12.2
            }
        } else {
            if (testFaceAmbiguity(TestTableCase12[configID][1])) {
                createTriangles(TilingTableCase12_2_Inverted[configID],
                                8); // 12.2
            } else {
                if (testInternalAmbiguity(caseID, configID, subconfigID,
                                          TestTableCase12[configID][2]))
                    createTriangles(TilingTableCase12_1_1[configID],
                                    4); // 12.1.1
                else
                    createTriangles(TilingTableCase12_1_2[configID],
                                    8); // 12.1.2
            }
        }
        break;

    case 13:
        if (testFaceAmbiguity(TestTableCase13[configID][0]))
            subconfigID += 1;
        if (testFaceAmbiguity(TestTableCase13[configID][1]))
            subconfigID += 2;
        if (testFaceAmbiguity(TestTableCase13[configID][2]))
            subconfigID += 4;
        if (testFaceAmbiguity(TestTableCase13[configID][3]))
            subconfigID += 8;
        if (testFaceAmbiguity(TestTableCase13[configID][4]))
            subconfigID += 16;
        if (testFaceAmbiguity(TestTableCase13[configID][5]))
            subconfigID += 32;
        switch (SubconfigTableCase13[subconfigID]) {
        case 0: /* 13.1 */
            createTriangles(TilingTableCase13_1[configID], 4);
            break;

        case 1: /* 13.2 */
            createTriangles(TilingTableCase13_2[configID][0], 6);
            break;
        case 2: /* 13.2 */
            createTriangles(TilingTableCase13_2[configID][1], 6);
            break;
        case 3: /* 13.2 */
            createTriangles(TilingTableCase13_2[configID][2], 6);
            break;
        case 4: /* 13.2 */
            createTriangles(TilingTableCase13_2[configID][3], 6);
            break;
        case 5: /* 13.2 */
            createTriangles(TilingTableCase13_2[configID][4], 6);
            break;
        case 6: /* 13.2 */
            createTriangles(TilingTableCase13_2[configID][5], 6);
            break;

        case 7: /* 13.3 */
            createTriangles(TilingTableCase13_3[configID][0], 10);
            break;
        case 8: /* 13.3 */
            createTriangles(TilingTableCase13_3[configID][1], 10);
            break;
        case 9: /* 13.3 */
            createTriangles(TilingTableCase13_3[configID][2], 10);
            break;
        case 10: /* 13.3 */
            createTriangles(TilingTableCase13_3[configID][3], 10);
            break;
        case 11: /* 13.3 */
            createTriangles(TilingTableCase13_3[configID][4], 10);
            break;
        case 12: /* 13.3 */
            createTriangles(TilingTableCase13_3[configID][5], 10);
            break;
        case 13: /* 13.3 */
            createTriangles(TilingTableCase13_3[configID][6], 10);
            break;
        case 14: /* 13.3 */
            createTriangles(TilingTableCase13_3[configID][7], 10);
            break;
        case 15: /* 13.3 */
            createTriangles(TilingTableCase13_3[configID][8], 10);
            break;
        case 16: /* 13.3 */
            createTriangles(TilingTableCase13_3[configID][9], 10);
            break;
        case 17: /* 13.3 */
            createTriangles(TilingTableCase13_3[configID][10], 10);
            break;
        case 18: /* 13.3 */
            createTriangles(TilingTableCase13_3[configID][11], 10);
            break;

        case 19: /* 13.4 */
            createTriangles(TilingTableCase13_4[configID][0], 12);
            break;
        case 20: /* 13.4 */
            createTriangles(TilingTableCase13_4[configID][1], 12);
            break;
        case 21: /* 13.4 */
            createTriangles(TilingTableCase13_4[configID][2], 12);
            break;
        case 22: /* 13.4 */
            createTriangles(TilingTableCase13_4[configID][3], 12);
            break;

        case 23: /* 13.5 */
            subconfigID = 0;
            if (testInternalAmbiguity(caseID, configID, subconfigID,
                                      TestTableCase13[configID][6]))
                createTriangles(TilingTableCase13_5_1[configID][0], 6);
            else
                createTriangles(TilingTableCase13_5_2[configID][0], 10);
            break;
        case 24: /* 13.5 */
            subconfigID = 1;
            if (testInternalAmbiguity(caseID, configID, subconfigID,
                                      TestTableCase13[configID][6]))
                createTriangles(TilingTableCase13_5_1[configID][1], 6);
            else
                createTriangles(TilingTableCase13_5_2[configID][1], 10);
            break;
        case 25: /* 13.5 */
            subconfigID = 2;
            if (testInternalAmbiguity(caseID, configID, subconfigID,
                                      TestTableCase13[configID][6]))
                createTriangles(TilingTableCase13_5_1[configID][2], 6);
            else
                createTriangles(TilingTableCase13_5_2[configID][2], 10);
            break;
        case 26: /* 13.5 */
            subconfigID = 3;
            if (testInternalAmbiguity(caseID, configID, subconfigID,
                                      TestTableCase13[configID][6]))
                createTriangles(TilingTableCase13_5_1[configID][3], 6);
            else
                createTriangles(TilingTableCase13_5_2[configID][3], 10);
            break;

        case 27: /* 13.3 */
            createTriangles(TilingTableCase13_3_Inverted[configID][0], 10);
            break;
        case 28: /* 13.3 */
            createTriangles(TilingTableCase13_3_Inverted[configID][1], 10);
            break;
        case 29: /* 13.3 */
            createTriangles(TilingTableCase13_3_Inverted[configID][2], 10);
            break;
        case 30: /* 13.3 */
            createTriangles(TilingTableCase13_3_Inverted[configID][3], 10);
            break;
        case 31: /* 13.3 */
            createTriangles(TilingTableCase13_3_Inverted[configID][4], 10);
            break;
        case 32: /* 13.3 */
            createTriangles(TilingTableCase13_3_Inverted[configID][5], 10);
            break;
        case 33: /* 13.3 */
            createTriangles(TilingTableCase13_3_Inverted[configID][6], 10);
            break;
        case 34: /* 13.3 */
            createTriangles(TilingTableCase13_3_Inverted[configID][7], 10);
            break;
        case 35: /* 13.3 */
            createTriangles(TilingTableCase13_3_Inverted[configID][8], 10);
            break;
        case 36: /* 13.3 */
            createTriangles(TilingTableCase13_3_Inverted[configID][9], 10);
            break;
        case 37: /* 13.3 */
            createTriangles(TilingTableCase13_3_Inverted[configID][10], 10);
            break;
        case 38: /* 13.3 */
            createTriangles(TilingTableCase13_3_Inverted[configID][11], 10);
            break;

        case 39: /* 13.2 */
            createTriangles(TilingTableCase13_2_Inverted[configID][0], 6);
            break;
        case 40: /* 13.2 */
            createTriangles(TilingTableCase13_2_Inverted[configID][1], 6);
            break;
        case 41: /* 13.2 */
            createTriangles(TilingTableCase13_2_Inverted[configID][2], 6);
            break;
        case 42: /* 13.2 */
            createTriangles(TilingTableCase13_2_Inverted[configID][3], 6);
            break;
        case 43: /* 13.2 */
            createTriangles(TilingTableCase13_2_Inverted[configID][4], 6);
            break;
        case 44: /* 13.2 */
            createTriangles(TilingTableCase13_2_Inverted[configID][5], 6);
            break;

        case 45: /* 13.1 */
            createTriangles(TilingTableCase13_1_Inverted[configID], 4);
            break;

        default: // Impossible Case
            assert(false);
        }
        break;

    case 14:
        createTriangles(TilingTableCase14[configID], 4);
        break;
    };
}

// Given a sequence of edges from the tiling table,
// generates triangles from them. This is used to generate the surface
// triangulation for our marching cube.
void MarchingCube::createTriangles(const char* edgeList, char numberTriangles) {
    Vector3 vertices[3];

    (*output_num_triangles) = numberTriangles;

    for (int i = 0; i < numberTriangles; i++) {
        for (int t = 0; t < 3; t++) {
            const char edgeID = edgeList[i * 3 + t];

            if (edgeID == 12) {
                Vector3 interiorPoint = Vector3();
                int count = 0;

                for (int v = 0; v < 12; v++) {
                    const Vector3 point = generateVertexOnEdge(v);

                    if (point.x != -1) {
                        interiorPoint += point;
                        count++;
                    }
                }

                interiorPoint /= count;

                vertices[t] = interiorPoint;
            } else if (0 <= edgeID && edgeID <= 11)
                vertices[t] = generateVertexOnEdge(edgeID);
            else // Invalid
                assert(false);
        }

        output_triangulation[i] =
            Triangle(vertices[0], vertices[1], vertices[2]);
    }
}

// Determines the coordinate of the terrain at some edgeID.
// Assumes that if we linearly interpolate, the value will hit 0
// along the edge.
Vector3 MarchingCube::generateVertexOnEdge(char edgeID) {
    // We have 2 points that form an edge - we sample the data at these points
    Vector3 basePoint, offset;
    float baseValue, offsetValue;

    // Based on the edge, determine the location of the edge's points,
    // and sample at these edge points
    switch (edgeID) {
    case 0:
        basePoint = Vector3(0, 0, 0);
        baseValue = vertexData[0];
        offset = Vector3(1, 0, 0);
        offsetValue = vertexData[1];
        break;
    case 1:
        basePoint = Vector3(1, 0, 0);
        baseValue = vertexData[1];
        offset = Vector3(0, 1, 0);
        offsetValue = vertexData[2];
        break;
    case 2:
        basePoint = Vector3(1, 1, 0);
        baseValue = vertexData[2];
        offset = Vector3(-1, 0, 0);
        offsetValue = vertexData[3];
        break;
    case 3:
        basePoint = Vector3(0, 1, 0);
        baseValue = vertexData[3];
        offset = Vector3(0, -1, 0);
        offsetValue = vertexData[0];
        break;
    case 4:
        basePoint = Vector3(0, 0, 1);
        baseValue = vertexData[4];
        offset = Vector3(1, 0, 0);
        offsetValue = vertexData[5];
        break;
    case 5:
        basePoint = Vector3(1, 0, 1);
        baseValue = vertexData[5];
        offset = Vector3(0, 1, 0);
        offsetValue = vertexData[6];
        break;
    case 6:
        basePoint = Vector3(1, 1, 1);
        baseValue = vertexData[6];
        offset = Vector3(-1, 0, 0);
        offsetValue = vertexData[7];
        break;
    case 7:
        basePoint = Vector3(0, 1, 1);
        baseValue = vertexData[7];
        offset = Vector3(0, -1, 0);
        offsetValue = vertexData[4];
        break;
    case 8:
        basePoint = Vector3(0, 0, 0);
        baseValue = vertexData[0];
        offset = Vector3(0, 0, 1);
        offsetValue = vertexData[4];
        break;
    case 9:
        basePoint = Vector3(1, 0, 0);
        baseValue = vertexData[1];
        offset = Vector3(0, 0, 1);
        offsetValue = vertexData[5];
        break;
    case 10:
        basePoint = Vector3(1, 1, 0);
        baseValue = vertexData[2];
        offset = Vector3(0, 0, 1);
        offsetValue = vertexData[6];
        break;
    case 11:
        basePoint = Vector3(0, 1, 0);
        baseValue = vertexData[3];
        offset = Vector3(0, 0, 1);
        offsetValue = vertexData[7];
        break;
    default: // INVALID EDGE ID
        assert(false);
    }

    // Linearly interpolate the location of the surface point
    // based on the sampled values
    float offset_percent = (-baseValue) / (offsetValue - baseValue);

    // assert(offset_percent != 0);

    // Determine surface coordinate
    if ((baseValue < 0 && offsetValue < 0) ||
        (baseValue > 0 && offsetValue > 0))
        return Vector3(-1, -1, -1);
    else
        return (basePoint + (offset * offset_percent));
}

// Given a face, returns if the center of the face is positive, as this can be
// an ambiguous case when the pairs of vertices on opposite diagonals have
// different signs. In this case, we could have the surface in the top left and
// bottom right corners, or in the top right and bottom left corners. Both are
// equally valid, and we need extra information to determine which is better.
//
// Example:
// 1 - - - -1      1 - - - -1
// |      \ |      | /      |
// | \     \|  or  |/      /|  ?
// |  \     |      |      / |
// -1 - - - 1      -1 - - - 1
//
// As we are linearly interpolating across the cube, we can test if the center
// of the face should be inside the surface or not. This information helps us
// resolve the ambiguity, telling us which corner the ambiguity resides on. This
// is also known as the asymptotic decider. A negative ID indicates that we need
// to flip the results of the test.
bool MarchingCube::testFaceAmbiguity(char faceID) {
    assert((1 <= faceID && faceID <= 6) || (-6 <= faceID && faceID <= -1));

    float a, b, c, d;

    // Given the face ID, load the data from the face's vertices
    switch (faceID) {
    case -1:
    case 1:
        a = vertexData[0];
        b = vertexData[4];
        c = vertexData[5];
        d = vertexData[1];
        break;

    case -2:
    case 2:
        a = vertexData[1];
        b = vertexData[5];
        c = vertexData[6];
        d = vertexData[2];
        break;

    case -3:
    case 3:
        a = vertexData[2];
        b = vertexData[6];
        c = vertexData[7];
        d = vertexData[3];
        break;

    case -4:
    case 4:
        a = vertexData[3];
        b = vertexData[7];
        c = vertexData[4];
        d = vertexData[0];
        break;

    case -5:
    case 5:
        a = vertexData[0];
        b = vertexData[3];
        c = vertexData[2];
        d = vertexData[1];
        break;

    case -6:
    case 6:
        a = vertexData[4];
        b = vertexData[7];
        c = vertexData[6];
        d = vertexData[5];
        break;

    default:
        assert(false);
        break;
    }

    // We multiply by faceID and a since their sign is important. If either
    // is negative, we need to invert the results of the test.
    if (std::abs(a * c - b * d) < FLT_EPSILON)
        return faceID >= 0;
    else
        return (faceID * a * (a * c - b * d)) >= 0;
}

// Given the cube, returns if the center of the cube is positive, as this can be
// an ambiguous case when the vertices on opposite diagonals are the same sign,
// all others the opposite sign. In this case, the vertices could be under the
// same surface (connected to one another through a tunnel), or under different
// surfaces which are not connected with one another in this voxel. This is a 3D
// generalization of the 2D face ambiguity above.
//
// Example:
//     -1________ 1
//      /|       /|
//    /  |     /  |
//-1/________/    |
// |     |  |-1   |
// |  -1 |__|_____| -1
// |    /   |    /
// |  /     |  /
// |/_______|/
// 1        -1
//
// Using our assumption that the values are linearly interpolated, we can
// determine the sign of the cube's center to resolve this ambiguity. If the
// points are connected through a tunnel, then there must exist some split in
// the voxel that the points are connected through. We can check for this split
// by solving for the plane where the 2D face case holds (the center is in the
// surface). Sign can either be 7 or -7, indicating if we should invert the
// results of the test or not. If 7, return true if the interior is empty. If
// -7, return false if the interior is empty.
bool MarchingCube::testInternalAmbiguity(char caseID, char configID,
                                         char subConfigID, char sign) {
    // Tracks the plane that should connect the points (if it does exist).
    float t, at = 0, bt = 0, ct = 0, dt = 0;
    // Caches some data for calculations
    float a, b;

    // Maintains the reference referenceEdge so we know what two opposite points
    // our ambiguous case is on.
    char referenceEdge = -1;

    switch (caseID) {
    case 4:
    case 10:
        a = (vertexData[4] - vertexData[0]) * (vertexData[6] - vertexData[2]) -
            (vertexData[7] - vertexData[3]) * (vertexData[5] - vertexData[1]);
        b = vertexData[2] * (vertexData[4] - vertexData[0]) +
            vertexData[0] * (vertexData[6] - vertexData[2]) -
            vertexData[1] * (vertexData[7] - vertexData[3]) -
            vertexData[3] * (vertexData[5] - vertexData[1]);
        t = -b / (2 * a);

        if (t < 0 || t > 1)
            return sign > 0;

        at = vertexData[0] + (vertexData[4] - vertexData[0]) * t;
        bt = vertexData[3] + (vertexData[7] - vertexData[3]) * t;
        ct = vertexData[2] + (vertexData[6] - vertexData[2]) * t;
        dt = vertexData[1] + (vertexData[5] - vertexData[1]) * t;
        break;

    case 6:
    case 7:
    case 12:
    case 13:
        switch (caseID) {
        case 6:
            referenceEdge = TestTableCase6[configID][2];
            break;
        case 7:
            referenceEdge = TestTableCase7[configID][4];
            break;
        case 12:
            referenceEdge = TestTableCase12[configID][3];
            break;
        case 13:
            referenceEdge = TilingTableCase13_5_1[configID][subConfigID][0];
            break;
        }
        switch (referenceEdge) {
        case 0:
            t = vertexData[0] / (vertexData[0] - vertexData[1]);
            at = 0;
            bt = vertexData[3] + (vertexData[2] - vertexData[3]) * t;
            ct = vertexData[7] + (vertexData[6] - vertexData[7]) * t;
            dt = vertexData[4] + (vertexData[5] - vertexData[4]) * t;
            break;
        case 1:
            t = vertexData[1] / (vertexData[1] - vertexData[2]);
            at = 0;
            bt = vertexData[0] + (vertexData[3] - vertexData[0]) * t;
            ct = vertexData[4] + (vertexData[7] - vertexData[4]) * t;
            dt = vertexData[5] + (vertexData[6] - vertexData[5]) * t;
            break;
        case 2:
            t = vertexData[2] / (vertexData[2] - vertexData[3]);
            at = 0;
            bt = vertexData[1] + (vertexData[0] - vertexData[1]) * t;
            ct = vertexData[5] + (vertexData[4] - vertexData[5]) * t;
            dt = vertexData[6] + (vertexData[7] - vertexData[6]) * t;
            break;
        case 3:
            t = vertexData[3] / (vertexData[3] - vertexData[0]);
            at = 0;
            bt = vertexData[2] + (vertexData[1] - vertexData[2]) * t;
            ct = vertexData[6] + (vertexData[5] - vertexData[6]) * t;
            dt = vertexData[7] + (vertexData[4] - vertexData[7]) * t;
            break;
        case 4:
            t = vertexData[4] / (vertexData[4] - vertexData[5]);
            at = 0;
            bt = vertexData[7] + (vertexData[6] - vertexData[7]) * t;
            ct = vertexData[3] + (vertexData[2] - vertexData[3]) * t;
            dt = vertexData[0] + (vertexData[1] - vertexData[0]) * t;
            break;
        case 5:
            t = vertexData[5] / (vertexData[5] - vertexData[6]);
            at = 0;
            bt = vertexData[4] + (vertexData[7] - vertexData[4]) * t;
            ct = vertexData[0] + (vertexData[3] - vertexData[0]) * t;
            dt = vertexData[1] + (vertexData[2] - vertexData[1]) * t;
            break;
        case 6:
            t = vertexData[6] / (vertexData[6] - vertexData[7]);
            at = 0;
            bt = vertexData[5] + (vertexData[4] - vertexData[5]) * t;
            ct = vertexData[1] + (vertexData[0] - vertexData[1]) * t;
            dt = vertexData[2] + (vertexData[3] - vertexData[2]) * t;
            break;
        case 7:
            t = vertexData[7] / (vertexData[7] - vertexData[4]);
            at = 0;
            bt = vertexData[6] + (vertexData[5] - vertexData[6]) * t;
            ct = vertexData[2] + (vertexData[1] - vertexData[2]) * t;
            dt = vertexData[3] + (vertexData[0] - vertexData[3]) * t;
            break;
        case 8:
            t = vertexData[0] / (vertexData[0] - vertexData[4]);
            at = 0;
            bt = vertexData[3] + (vertexData[7] - vertexData[3]) * t;
            ct = vertexData[2] + (vertexData[6] - vertexData[2]) * t;
            dt = vertexData[1] + (vertexData[5] - vertexData[1]) * t;
            break;
        case 9:
            t = vertexData[1] / (vertexData[1] - vertexData[5]);
            at = 0;
            bt = vertexData[0] + (vertexData[4] - vertexData[0]) * t;
            ct = vertexData[3] + (vertexData[7] - vertexData[3]) * t;
            dt = vertexData[2] + (vertexData[6] - vertexData[2]) * t;
            break;
        case 10:
            t = vertexData[2] / (vertexData[2] - vertexData[6]);
            at = 0;
            bt = vertexData[1] + (vertexData[5] - vertexData[1]) * t;
            ct = vertexData[0] + (vertexData[4] - vertexData[0]) * t;
            dt = vertexData[3] + (vertexData[7] - vertexData[3]) * t;
            break;
        case 11:
            t = vertexData[3] / (vertexData[3] - vertexData[7]);
            at = 0;
            bt = vertexData[2] + (vertexData[6] - vertexData[2]) * t;
            ct = vertexData[1] + (vertexData[5] - vertexData[1]) * t;
            dt = vertexData[0] + (vertexData[4] - vertexData[0]) * t;
            break;

        // Invalid Reference Edge
        default:
            assert(false);
            break;
        }
        break;

    // Invalid Ambiguous Case
    default:
        assert(false);
        break;
    }

    // Determine what test we need to perform
    char test = 0;

    if (at >= 0)
        test++;
    if (bt >= 0)
        test += 2;
    if (ct >= 0)
        test += 4;
    if (dt >= 0)
        test += 8;

    switch (test) {
    case 0:
        return sign > 0;
    case 1:
        return sign > 0;
    case 2:
        return sign > 0;
    case 3:
        return sign > 0;
    case 4:
        return sign > 0;
    case 5:
        if (at * ct - bt * dt < FLT_EPSILON)
            return sign > 0;
        break;
    case 6:
        return sign > 0;
    case 7:
        return sign < 0;
    case 8:
        return sign > 0;
    case 9:
        return sign > 0;
    case 10:
        if (at * ct - bt * dt >= 0)
            return sign > 0;
        break;
    case 11:
        return sign < 0;
    case 12:
        return sign > 0;
    case 13:
        return sign < 0;
    case 14:
        return sign < 0;
    case 15:
        return sign < 0;
    }

    return sign < 0;
}

// Generate a mask for the cube indicating what vertices of the voxel are inside
// the surface, and what vertices are outside. Voxels with bit flipped to 1 are
// inside the surface, and voxels with bit flipped to 0 are outside the surface.
char MarchingCube::computeVertexMask() {
    unsigned char mask = 0;

    // For each vertex in the voxel, test if it is inside the surface by
    // checking if its data value is positive. Add this result to the bitmask.
    for (int i = 0; i < 8; i++)
        if (vertexData[i] > 0)
            mask |= 1 << i;

    return mask;
}

} // namespace Graphics
} // namespace Engine