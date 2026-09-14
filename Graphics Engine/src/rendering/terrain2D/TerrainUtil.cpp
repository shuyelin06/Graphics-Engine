#define NOMINMAX
#include "TerrainUtil.h"

#include <algorithm>

namespace Engine
{
namespace Graphics
{
void generateTerrainPlaneMesh(MeshBuilder& builder,
                              int numSamples,
                              float skirtDepth)
{
    builder.reset();
    builder.addLayout(PosXYZ_TexU);

    // Force num samples to at least 2 so we can at create a flat square
    numSamples = std::max(2, numSamples);

    // Generate a very simple mesh with bounds
    // - x,z in [0,1]
    // - y = 0

    // Step 1) Generate a grid of points in the order of
    // 6 7 8...
    // 3 4 5
    // 0 1 2
    // Bottom left corner is (x,z) = (0,0). Right is +x, Up is +z.
    const float sampleDistanceInv = 1 / float(numSamples - 1);
    for (int sampleX = 0; sampleX < numSamples; sampleX++)
    {
        for (int sampleZ = 0; sampleZ < numSamples; sampleZ++)
        {
            const float x = sampleX * sampleDistanceInv;
            const float y = 0.f;
            const float z = sampleZ * sampleDistanceInv;
            builder.addVertex(Vector3(x, y, z));
        }
    }

    // Connect my points together. For a given quad in the grid, a,b,c,d
    // reference indices as so
    // d c
    // a b
    for (int indexX = 0; indexX < numSamples - 1; indexX++)
    {
        for (int indexZ = 0; indexZ < numSamples - 1; indexZ++)
        {
            const unsigned int a = indexZ + indexX * numSamples;
            const unsigned int b = indexZ + (indexX + 1) * numSamples;
            const unsigned int c = (indexZ + 1) + (indexX + 1) * numSamples;
            const unsigned int d = (indexZ + 1) + indexX * numSamples;
            builder.addTriangle(a, d, b);
            builder.addTriangle(c, b, d);
        }
    }

    // Generate a skirt. This is a set of vertices that protrude downwards from
    // the edges of the terrain mesh. Skirts are a cheap and simple way to hide
    // the LOD transitions.
    if (skirtDepth > 0.f)
    {
        const unsigned int skirtIndexStart = builder.getVertices().size();

        std::vector<unsigned int> borderIndices;
        auto generateSkirtVerticesAlongBorder = [&](unsigned int startX,
                                                    unsigned int startZ,
                                                    int offsetX, int offsetZ) {
            for (int i = 0; i < numSamples; i++)
            {
                const unsigned int indexX = startX + offsetX * i;
                const unsigned int indexZ = startZ + offsetZ * i;

                const unsigned int vertexIndex = indexZ + indexX * numSamples;
                borderIndices.push_back(vertexIndex);

                const Vector3 skirtVertex =
                    builder.getVertex(vertexIndex).position +
                    Vector3(0, -skirtDepth, 0);
                builder.addVertex(skirtVertex);
            }
        };

        // Walk the grid border counter-clockwise and generate the skirt
        // vertices
        generateSkirtVerticesAlongBorder(0, 0, 1, 0);
        generateSkirtVerticesAlongBorder(numSamples - 1, 0, 0, 1);
        generateSkirtVerticesAlongBorder(numSamples - 1, numSamples - 1, -1, 0);
        generateSkirtVerticesAlongBorder(0, numSamples - 1, 0, -1);

        assert(builder.getVertices().size() - skirtIndexStart ==
               borderIndices.size());
        for (int i = 0; i < borderIndices.size() - 1; i++)
        {
            const unsigned int a = borderIndices[i];
            const unsigned int b = borderIndices[i + 1];
            const unsigned int c = skirtIndexStart + i + 1;
            const unsigned int d = skirtIndexStart + i;

            builder.addTriangle(a, b, d);
            builder.addTriangle(c, d, b);
        };
    }
}

} // namespace Graphics
} // namespace Engine