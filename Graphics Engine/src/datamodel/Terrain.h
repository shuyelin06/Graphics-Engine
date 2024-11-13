#pragma once

#include "math/Vector3.h"

#include "rendering/components/Asset.h"

// Enables debug rendering for the terrain
// #define TERRAIN_DEBUG

#define CHUNK_X_SIZE 25
#define CHUNK_Y_SIZE 10
#define CHUNK_Z_SIZE 25
#define CHUNK_VOXEL_SIZE 75

namespace Engine
{
using namespace Math;
using namespace Graphics;
namespace Datamodel
{
	// Terrain Class:
	// Represents a chunk of terrain that can be sampled and rendered.
	// Terrain generation is done by running the Marching Cubes algorithm on some
	// defined noise function. 
	// It generates the best surface for the data by assuming linear interpolation, and
	// generating a surface where the data will approximately be 0. Data that is postive is assumed
	// to be inside the surface, and data that is negative is assumed to be outside the surface.
	class Terrain
	{
	private:
		// Stores a scalar field's values at vertices of each voxel within the chunk.
		float terrainData[CHUNK_X_SIZE][CHUNK_Y_SIZE][CHUNK_Z_SIZE];

		// Terrain mesh
		Asset* mesh;

	public:
		Terrain();
		~Terrain();

		Asset* getMesh();
		void generateMesh();
	};
}
}
