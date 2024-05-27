#pragma once

#include "math/Vector3.h"

#include "rendering/Mesh.h"

namespace Engine
{
using namespace Math;
using namespace Graphics;
namespace Datamodel
{
	class Terrain
	{
	private:
		int x_size, y_size, z_size;

		float*** grid;

		float voxel_size;
		float surface_level;

		Mesh mesh;

	public:
		Terrain(int x, int y, int z, float voxel_size);
		~Terrain();

		// Gets the terrain mesh
		Mesh& getMesh();

		// Sample value at point
		float samplePoint(int x, int y, int z);

		// DEBUG: Test the configuration
		void checkConfiguration(int mask);

	private:
		void generateMesh();

		// Finds the edge mask at the cube with bottom-left corner
		// given at (x,y,z)
		int edgeMask(int x, int y, int z);

		// Determines the coordinates of the edge identifier in the
		// cube given at (x,y,z)
		Vector3 terrainCoordinate(int x, int y, int z, int edge_ID);

		
	};
}
}
