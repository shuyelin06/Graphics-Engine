#pragma once

#include "math/Vector3.h"

#include "rendering/Asset.h"

#define TERRAIN_X_SIZE 15
#define TERRAIN_Y_SIZE 15
#define TERRAIN_Z_SIZE 15
#define TERRAIN_VOXEL_SIZE 5

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
		float terrainData[TERRAIN_X_SIZE][TERRAIN_Y_SIZE][TERRAIN_Z_SIZE];

		// UNUSED
		Mesh mesh;

	public:
		/*
		void generateMesh();

		Terrain(int x, int y, int z, float voxel_size);
		~Terrain();

		
		// Gets the terrain mesh
		Mesh& getMesh();

		// Sample value at point
		float samplePoint(int x, int y, int z);

		// DEBUG: Test the configuration
		void checkConfiguration(int mask);

	private:
		// Helper methods for the Marching Cube implementation
		void processVoxel(int x, int y, int z);
		
		bool testInteriorAmbiguity();
		bool testFaceAmbiguity(int x, int y, int z);

		void addTriangle(const char* triangleList, char n, int v12 = -1);
		bool faceContainsSurface(int x, int y, int z, char faceID);
		float sampleVoxelData(int x, int y, int z, char vertexID);
		char computeVertexMask(int x, int y, int z);

		


		void generateMesh();

		// Finds the edge mask at the cube with bottom-left corner
		// given at (x,y,z)
		int edgeMask(int x, int y, int z);

		// Determines the coordinates of the edge identifier in the
		// cube given at (x,y,z)
		Vector3 terrainCoordinate(int x, int y, int z, int edge_ID);

		*/
	};
}
}
