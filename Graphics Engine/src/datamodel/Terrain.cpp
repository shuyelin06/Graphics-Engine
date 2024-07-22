#include "Terrain.h"

#include "TerrainLookupTables.h"

#include "math/Compute.h"
#include <assert.h>
#include <map>

#include "math/Matrix3.h"

namespace Engine
{
namespace Datamodel
{
	// Terrain Generation using an adapted version of Lewiner, et. al's 
	// 3D Marching Cubes implementation.
	// http://thomas.lewiner.org/pdfs/marching_cubes_jgt.pdf
	
	/* 
	* Cube Mappings:
	* X-axis goes left to right, Z-axis goes bottom to top, Y-axis goes back to front.
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

	/*
	// MarchingCube Class:
	// Represents a cube with float values at each of its 8 vertices. These float values can be positive
	// or negative, and we assume that a surface exists where these values are 0 (after linearly interpolating the
	// vertex values across the entire cube).
	// This class implements the marching cubes algorithm to generate a non-ambiguous triangulation for a surface approximating
	// where these values are 0. 
	class MarchingCube
	{
	private:
		// Data is given in order of the vertex id mapping shown above
		float vertexData[8];
	
	public:
		MarchingCube();
		~MarchingCube();

		void generateSurface();

	private:
		bool testFaceAmbiguity(char faceID);
		bool testInternalAmbiguity(char sign);

		char computeVertexMask();
	};

	// Given a face, returns if the center of the face is positive, as this can be an ambiguous case
	// when the pairs of vertices on opposite diagonals have different signs.
	// As we are linearly interpolating across the cube, we can test if the center of the face should be inside the surface or not.
	// This information helps us resolve the ambiguity. This is known as the asymptotic decider.
	// A negative ID indicates that we need to flip the results of the test.
	bool MarchingCube::testFaceAmbiguity(char faceID)
	{
		float a, b, c, d;

		// Given the face ID, load the data from the face's vertices
		switch (faceID)
		{
		case -1: case 1:
			a = vertexData[0]; b = vertexData[4]; c = vertexData[5]; d = vertexData[1];
			break;

		case -2: case 2:
			a = vertexData[1]; b = vertexData[5]; c = vertexData[6]; d = vertexData[2];
			break;

		case -3:
		case 3:
			a = vertexData[2]; b = vertexData[6]; c = vertexData[7]; d = vertexData[3];
			break;

		case -4:
		case 4:
			a = vertexData[3]; b = vertexData[7]; c = vertexData[4]; d = vertexData[0];
			break;

		case -5:
		case 5:
			a = vertexData[0]; b = vertexData[3]; c = vertexData[2]; d = vertexData[1];
			break;

		case -6:
		case 6:
			a = vertexData[4]; b = vertexData[7]; c = vertexData[6]; d = vertexData[5];
			break;
		}

		return faceID * a * (a * c - b * d) >= 0;
	}

	// Given the cube, returns if the center of the cube is positive, as this can be an ambiguous case
	// when the vertices on opposite diagonals are the same sign, all others the opposite sign.
	// In this case, the vertices could be connected through a tunnel, or disconnected.
	// Using our assumption that the values are linearly interpolated, we can determine the sign of the cube's center
	// to resolve this ambiguity.
	// Sign can either be 7 or -7, indicating if we should invert the results of the test or not.
	// If 7, return true if the interior is empty. If -7, return false if the interior is empty.
	bool MarchingCube::testInternalAmbiguity(char caseID, char sign)
	{
		float at = 0, bt = 0, ct = 0, dt = 0, t, a, b;

		// Based on our case, 
		real t, At = 0, Bt = 0, Ct = 0, Dt = 0, a, b;
		char  test = 0;
		char  edge = -1; // reference edge of the triangulation

		switch (_case)
		{
		case  4:
		case 10:
			a = (_cube[4] - _cube[0]) * (_cube[6] - _cube[2]) - (_cube[7] - _cube[3]) * (_cube[5] - _cube[1]);
			b = _cube[2] * (_cube[4] - _cube[0]) + _cube[0] * (_cube[6] - _cube[2])
				- _cube[1] * (_cube[7] - _cube[3]) - _cube[3] * (_cube[5] - _cube[1]);
			t = -b / (2 * a);
			if (t < 0 || t>1) return s > 0;

			At = _cube[0] + (_cube[4] - _cube[0]) * t;
			Bt = _cube[3] + (_cube[7] - _cube[3]) * t;
			Ct = _cube[2] + (_cube[6] - _cube[2]) * t;
			Dt = _cube[1] + (_cube[5] - _cube[1]) * t;
			break;

		case  6:
		case  7:
		case 12:
		case 13:
			switch (_case)
			{
			case  6: edge = test6[_config][2]; break;
			case  7: edge = test7[_config][4]; break;
			case 12: edge = test12[_config][3]; break;
			case 13: edge = tiling13_5_1[_config][_subconfig][0]; break;
			}
			switch (edge)
			{
			case  0:
				t = _cube[0] / (_cube[0] - _cube[1]);
				At = 0;
				Bt = _cube[3] + (_cube[2] - _cube[3]) * t;
				Ct = _cube[7] + (_cube[6] - _cube[7]) * t;
				Dt = _cube[4] + (_cube[5] - _cube[4]) * t;
				break;
			case  1:
				t = _cube[1] / (_cube[1] - _cube[2]);
				At = 0;
				Bt = _cube[0] + (_cube[3] - _cube[0]) * t;
				Ct = _cube[4] + (_cube[7] - _cube[4]) * t;
				Dt = _cube[5] + (_cube[6] - _cube[5]) * t;
				break;
			case  2:
				t = _cube[2] / (_cube[2] - _cube[3]);
				At = 0;
				Bt = _cube[1] + (_cube[0] - _cube[1]) * t;
				Ct = _cube[5] + (_cube[4] - _cube[5]) * t;
				Dt = _cube[6] + (_cube[7] - _cube[6]) * t;
				break;
			case  3:
				t = _cube[3] / (_cube[3] - _cube[0]);
				At = 0;
				Bt = _cube[2] + (_cube[1] - _cube[2]) * t;
				Ct = _cube[6] + (_cube[5] - _cube[6]) * t;
				Dt = _cube[7] + (_cube[4] - _cube[7]) * t;
				break;
			case  4:
				t = _cube[4] / (_cube[4] - _cube[5]);
				At = 0;
				Bt = _cube[7] + (_cube[6] - _cube[7]) * t;
				Ct = _cube[3] + (_cube[2] - _cube[3]) * t;
				Dt = _cube[0] + (_cube[1] - _cube[0]) * t;
				break;
			case  5:
				t = _cube[5] / (_cube[5] - _cube[6]);
				At = 0;
				Bt = _cube[4] + (_cube[7] - _cube[4]) * t;
				Ct = _cube[0] + (_cube[3] - _cube[0]) * t;
				Dt = _cube[1] + (_cube[2] - _cube[1]) * t;
				break;
			case  6:
				t = _cube[6] / (_cube[6] - _cube[7]);
				At = 0;
				Bt = _cube[5] + (_cube[4] - _cube[5]) * t;
				Ct = _cube[1] + (_cube[0] - _cube[1]) * t;
				Dt = _cube[2] + (_cube[3] - _cube[2]) * t;
				break;
			case  7:
				t = _cube[7] / (_cube[7] - _cube[4]);
				At = 0;
				Bt = _cube[6] + (_cube[5] - _cube[6]) * t;
				Ct = _cube[2] + (_cube[1] - _cube[2]) * t;
				Dt = _cube[3] + (_cube[0] - _cube[3]) * t;
				break;
			case  8:
				t = _cube[0] / (_cube[0] - _cube[4]);
				At = 0;
				Bt = _cube[3] + (_cube[7] - _cube[3]) * t;
				Ct = _cube[2] + (_cube[6] - _cube[2]) * t;
				Dt = _cube[1] + (_cube[5] - _cube[1]) * t;
				break;
			case  9:
				t = _cube[1] / (_cube[1] - _cube[5]);
				At = 0;
				Bt = _cube[0] + (_cube[4] - _cube[0]) * t;
				Ct = _cube[3] + (_cube[7] - _cube[3]) * t;
				Dt = _cube[2] + (_cube[6] - _cube[2]) * t;
				break;
			case 10:
				t = _cube[2] / (_cube[2] - _cube[6]);
				At = 0;
				Bt = _cube[1] + (_cube[5] - _cube[1]) * t;
				Ct = _cube[0] + (_cube[4] - _cube[0]) * t;
				Dt = _cube[3] + (_cube[7] - _cube[3]) * t;
				break;
			case 11:
				t = _cube[3] / (_cube[3] - _cube[7]);
				At = 0;
				Bt = _cube[2] + (_cube[6] - _cube[2]) * t;
				Ct = _cube[1] + (_cube[5] - _cube[1]) * t;
				Dt = _cube[0] + (_cube[4] - _cube[0]) * t;
				break;
			default: printf("Invalid edge %d\n", edge);  print_cube();  break;
			}
			break;

		default: printf("Invalid ambiguous case %d\n", _case);  print_cube();  break;
		}

		if (At >= 0) test++;
		if (Bt >= 0) test += 2;
		if (Ct >= 0) test += 4;
		if (Dt >= 0) test += 8;
		switch (test)
		{
		case  0: return s > 0;
		case  1: return s > 0;
		case  2: return s > 0;
		case  3: return s > 0;
		case  4: return s > 0;
		case  5: if (At* Ct - Bt * Dt < FLT_EPSILON) return s > 0; break;
		case  6: return s > 0;
		case  7: return s < 0;
		case  8: return s > 0;
		case  9: return s > 0;
		case 10: if (At * Ct - Bt * Dt >= FLT_EPSILON) return s > 0; break;
		case 11: return s < 0;
		case 12: return s > 0;
		case 13: return s < 0;
		case 14: return s < 0;
		case 15: return s < 0;
		}

		return s < 0;
	}


	// Generate a mask for the cube indicating what vertices of the voxel are inside
	// the surface, and what vertices are outside. Voxels with bit flipped to 1 are inside the surface,
	// and voxels with bit flipped to 0 are outside the surface.
	char MarchingCube::computeVertexMask()
	{
		char mask = 0;

		// For each vertex in the voxel, test if it is inside the surface by checking
		// if its data value is positive. Add this result to the bitmask.
		for (int i = 0; i < 8; i++)
			mask |= (vertexData[i] >= 0) << i;

		return mask;
	}

	// Triangulation for a given voxel. Using Lewiner's implementation, this triangulation is guaranteed
	// not to have ambiguities and will be watertight with other adjacent voxels.
	void MarchingCube::generateSurface()
	{
		// Compute the voxel's vertex mask, and use it to determine the case
		// we need to process and configuration type.
		const int vertexMask = computeVertexMask(x, y, z);

		const char caseID = CaseTable[vertexMask][0];
		const char configID = CaseTable[vertexMask][0];
		
		// On a case-by-case basis, determine how we should triangulate this voxel.
		// There are 15 main cases to test, but they vary in complexity as some cases may have ambiguity
		// we need to test against.
		// Some cases may even have sub-cases which we need to test.
		switch (caseID)
		{
		case  0:
			break;

		case  1:
			addTriangle(TilingTableCase1[configID], 1);
			break;

		case  2:
			addTriangle(TilingTableCase2[configID], 2);
			break;

		case  3:
			if (faceContainsSurface(x, y, z, TestTableCase3[configID]))
				addTriangle(TilingTableCase3_2[configID], 4); // 3.2
			else
				addTriangle(TilingTableCase3_1[configID], 2); // 3.1
			break;

	}


	// 
	// TODO:
	// https://github.com/weshoke/efficient-marching-cubes/blob/master/src/LookUpTable.h
	// The original Marching Cubes is flawed. It has ambiguities that lead to holes in the terrain.
	// https://www.ks.uiuc.edu/Research/vmd/projects/ece498/surf/lewiner.pdf
	// There are many methods created to address this. This is one of them, extending the original technique.
	// Can also consider Marching Tetrahedron
	// More complex techniques are discussed here
	// https://transvoxel.org/Lengyel-VoxelTerrain.pdf
	// https://i.sstatic.net/2ndOx.png

	// Vertex and edge layout:
    //
    //            6             7
    //            +-------------+               +-----6-------+   
    //          / |           / |             / |            /|   
    //        /   |         /   |          11   7         10   5
    //    2 +-----+-------+  3  |         +------2------+     |   
    //      |   4 +-------+-----+ 5       |     +-----4-|-----+   
    //      |   /         |   /           3   8         1   9
    //      | /           | /             | /           | /       
    //    0 +-------------+ 1             +------0------+   

    // Data courtesy of https://gist.github.com/dwilliamson/c041e3454a713e58baf6e4f8e5fffecd
    
    // For each MC case, a list of triangles, specified as triples of edge indices, terminated by -1
    const std::vector<std::vector<int>> TriangleTable = 
	{
		{-1},
		{0, 3, 8, -1},
		{0, 9, 1, -1},
		{3, 8, 1, 1, 8, 9, -1},
		{2, 11, 3, -1},
		{8, 0, 11, 11, 0, 2, -1},
		{3, 2, 11, 1, 0, 9, -1},
		{11, 1, 2, 11, 9, 1, 11, 8, 9, -1},
		{1, 10, 2, -1},
		{0, 3, 8, 2, 1, 10, -1},
		{10, 2, 9, 9, 2, 0, -1},
		{8, 2, 3, 8, 10, 2, 8, 9, 10, -1},
		{11, 3, 10, 10, 3, 1, -1},
		{10, 0, 1, 10, 8, 0, 10, 11, 8, -1},
		{9, 3, 0, 9, 11, 3, 9, 10, 11, -1},
		{8, 9, 11, 11, 9, 10, -1},
		{4, 8, 7, -1},
		{7, 4, 3, 3, 4, 0, -1},
		{4, 8, 7, 0, 9, 1, -1},
		{1, 4, 9, 1, 7, 4, 1, 3, 7, -1},
		{8, 7, 4, 11, 3, 2, -1},
		{4, 11, 7, 4, 2, 11, 4, 0, 2, -1},
		{0, 9, 1, 8, 7, 4, 11, 3, 2, -1},
		{7, 4, 11, 11, 4, 2, 2, 4, 9, 2, 9, 1, -1},
		{4, 8, 7, 2, 1, 10, -1},
		{7, 4, 3, 3, 4, 0, 10, 2, 1, -1},
		{10, 2, 9, 9, 2, 0, 7, 4, 8, -1},
		{10, 2, 3, 10, 3, 4, 3, 7, 4, 9, 10, 4, -1},
		{1, 10, 3, 3, 10, 11, 4, 8, 7, -1},
		{10, 11, 1, 11, 7, 4, 1, 11, 4, 1, 4, 0, -1},
		{7, 4, 8, 9, 3, 0, 9, 11, 3, 9, 10, 11, -1},
		{7, 4, 11, 4, 9, 11, 9, 10, 11, -1},
		{9, 4, 5, -1},
		{9, 4, 5, 8, 0, 3, -1},
		{4, 5, 0, 0, 5, 1, -1},
		{5, 8, 4, 5, 3, 8, 5, 1, 3, -1},
		{9, 4, 5, 11, 3, 2, -1},
		{2, 11, 0, 0, 11, 8, 5, 9, 4, -1},
		{4, 5, 0, 0, 5, 1, 11, 3, 2, -1},
		{5, 1, 4, 1, 2, 11, 4, 1, 11, 4, 11, 8, -1},
		{1, 10, 2, 5, 9, 4, -1},
		{9, 4, 5, 0, 3, 8, 2, 1, 10, -1},
		{2, 5, 10, 2, 4, 5, 2, 0, 4, -1},
		{10, 2, 5, 5, 2, 4, 4, 2, 3, 4, 3, 8, -1},
		{11, 3, 10, 10, 3, 1, 4, 5, 9, -1},
		{4, 5, 9, 10, 0, 1, 10, 8, 0, 10, 11, 8, -1},
		{11, 3, 0, 11, 0, 5, 0, 4, 5, 10, 11, 5, -1},
		{4, 5, 8, 5, 10, 8, 10, 11, 8, -1},
		{8, 7, 9, 9, 7, 5, -1},
		{3, 9, 0, 3, 5, 9, 3, 7, 5, -1},
		{7, 0, 8, 7, 1, 0, 7, 5, 1, -1},
		{7, 5, 3, 3, 5, 1, -1},
		{5, 9, 7, 7, 9, 8, 2, 11, 3, -1},
		{2, 11, 7, 2, 7, 9, 7, 5, 9, 0, 2, 9, -1},
		{2, 11, 3, 7, 0, 8, 7, 1, 0, 7, 5, 1, -1},
		{2, 11, 1, 11, 7, 1, 7, 5, 1, -1},
		{8, 7, 9, 9, 7, 5, 2, 1, 10, -1},
		{10, 2, 1, 3, 9, 0, 3, 5, 9, 3, 7, 5, -1},
		{7, 5, 8, 5, 10, 2, 8, 5, 2, 8, 2, 0, -1},
		{10, 2, 5, 2, 3, 5, 3, 7, 5, -1},
		{8, 7, 5, 8, 5, 9, 11, 3, 10, 3, 1, 10, -1},
		{5, 11, 7, 10, 11, 5, 1, 9, 0, -1},
		{11, 5, 10, 7, 5, 11, 8, 3, 0, -1},
		{5, 11, 7, 10, 11, 5, -1},
		{6, 7, 11, -1},
		{7, 11, 6, 3, 8, 0, -1},
		{6, 7, 11, 0, 9, 1, -1},
		{9, 1, 8, 8, 1, 3, 6, 7, 11, -1},
		{3, 2, 7, 7, 2, 6, -1},
		{0, 7, 8, 0, 6, 7, 0, 2, 6, -1},
		{6, 7, 2, 2, 7, 3, 9, 1, 0, -1},
		{6, 7, 8, 6, 8, 1, 8, 9, 1, 2, 6, 1, -1},
		{11, 6, 7, 10, 2, 1, -1},
		{3, 8, 0, 11, 6, 7, 10, 2, 1, -1},
		{0, 9, 2, 2, 9, 10, 7, 11, 6, -1},
		{6, 7, 11, 8, 2, 3, 8, 10, 2, 8, 9, 10, -1},
		{7, 10, 6, 7, 1, 10, 7, 3, 1, -1},
		{8, 0, 7, 7, 0, 6, 6, 0, 1, 6, 1, 10, -1},
		{7, 3, 6, 3, 0, 9, 6, 3, 9, 6, 9, 10, -1},
		{6, 7, 10, 7, 8, 10, 8, 9, 10, -1},
		{11, 6, 8, 8, 6, 4, -1},
		{6, 3, 11, 6, 0, 3, 6, 4, 0, -1},
		{11, 6, 8, 8, 6, 4, 1, 0, 9, -1},
		{1, 3, 9, 3, 11, 6, 9, 3, 6, 9, 6, 4, -1},
		{2, 8, 3, 2, 4, 8, 2, 6, 4, -1},
		{4, 0, 6, 6, 0, 2, -1},
		{9, 1, 0, 2, 8, 3, 2, 4, 8, 2, 6, 4, -1},
		{9, 1, 4, 1, 2, 4, 2, 6, 4, -1},
		{4, 8, 6, 6, 8, 11, 1, 10, 2, -1},
		{1, 10, 2, 6, 3, 11, 6, 0, 3, 6, 4, 0, -1},
		{11, 6, 4, 11, 4, 8, 10, 2, 9, 2, 0, 9, -1},
		{10, 4, 9, 6, 4, 10, 11, 2, 3, -1},
		{4, 8, 3, 4, 3, 10, 3, 1, 10, 6, 4, 10, -1},
		{1, 10, 0, 10, 6, 0, 6, 4, 0, -1},
		{4, 10, 6, 9, 10, 4, 0, 8, 3, -1},
		{4, 10, 6, 9, 10, 4, -1},
		{6, 7, 11, 4, 5, 9, -1},
		{4, 5, 9, 7, 11, 6, 3, 8, 0, -1},
		{1, 0, 5, 5, 0, 4, 11, 6, 7, -1},
		{11, 6, 7, 5, 8, 4, 5, 3, 8, 5, 1, 3, -1},
		{3, 2, 7, 7, 2, 6, 9, 4, 5, -1},
		{5, 9, 4, 0, 7, 8, 0, 6, 7, 0, 2, 6, -1},
		{3, 2, 6, 3, 6, 7, 1, 0, 5, 0, 4, 5, -1},
		{6, 1, 2, 5, 1, 6, 4, 7, 8, -1},
		{10, 2, 1, 6, 7, 11, 4, 5, 9, -1},
		{0, 3, 8, 4, 5, 9, 11, 6, 7, 10, 2, 1, -1},
		{7, 11, 6, 2, 5, 10, 2, 4, 5, 2, 0, 4, -1},
		{8, 4, 7, 5, 10, 6, 3, 11, 2, -1},
		{9, 4, 5, 7, 10, 6, 7, 1, 10, 7, 3, 1, -1},
		{10, 6, 5, 7, 8, 4, 1, 9, 0, -1},
		{4, 3, 0, 7, 3, 4, 6, 5, 10, -1},
		{10, 6, 5, 8, 4, 7, -1},
		{9, 6, 5, 9, 11, 6, 9, 8, 11, -1},
		{11, 6, 3, 3, 6, 0, 0, 6, 5, 0, 5, 9, -1},
		{11, 6, 5, 11, 5, 0, 5, 1, 0, 8, 11, 0, -1},
		{11, 6, 3, 6, 5, 3, 5, 1, 3, -1},
		{9, 8, 5, 8, 3, 2, 5, 8, 2, 5, 2, 6, -1},
		{5, 9, 6, 9, 0, 6, 0, 2, 6, -1},
		{1, 6, 5, 2, 6, 1, 3, 0, 8, -1},
		{1, 6, 5, 2, 6, 1, -1},
		{2, 1, 10, 9, 6, 5, 9, 11, 6, 9, 8, 11, -1},
		{9, 0, 1, 3, 11, 2, 5, 10, 6, -1},
		{11, 0, 8, 2, 0, 11, 10, 6, 5, -1},
		{3, 11, 2, 5, 10, 6, -1},
		{1, 8, 3, 9, 8, 1, 5, 10, 6, -1},
		{6, 5, 10, 0, 1, 9, -1},
		{8, 3, 0, 5, 10, 6, -1},
		{6, 5, 10, -1},
		{10, 5, 6, -1},
		{0, 3, 8, 6, 10, 5, -1},
		{10, 5, 6, 9, 1, 0, -1},
		{3, 8, 1, 1, 8, 9, 6, 10, 5, -1},
		{2, 11, 3, 6, 10, 5, -1},
		{8, 0, 11, 11, 0, 2, 5, 6, 10, -1},
		{1, 0, 9, 2, 11, 3, 6, 10, 5, -1},
		{5, 6, 10, 11, 1, 2, 11, 9, 1, 11, 8, 9, -1},
		{5, 6, 1, 1, 6, 2, -1},
		{5, 6, 1, 1, 6, 2, 8, 0, 3, -1},
		{6, 9, 5, 6, 0, 9, 6, 2, 0, -1},
		{6, 2, 5, 2, 3, 8, 5, 2, 8, 5, 8, 9, -1},
		{3, 6, 11, 3, 5, 6, 3, 1, 5, -1},
		{8, 0, 1, 8, 1, 6, 1, 5, 6, 11, 8, 6, -1},
		{11, 3, 6, 6, 3, 5, 5, 3, 0, 5, 0, 9, -1},
		{5, 6, 9, 6, 11, 9, 11, 8, 9, -1},
		{5, 6, 10, 7, 4, 8, -1},
		{0, 3, 4, 4, 3, 7, 10, 5, 6, -1},
		{5, 6, 10, 4, 8, 7, 0, 9, 1, -1},
		{6, 10, 5, 1, 4, 9, 1, 7, 4, 1, 3, 7, -1},
		{7, 4, 8, 6, 10, 5, 2, 11, 3, -1},
		{10, 5, 6, 4, 11, 7, 4, 2, 11, 4, 0, 2, -1},
		{4, 8, 7, 6, 10, 5, 3, 2, 11, 1, 0, 9, -1},
		{1, 2, 10, 11, 7, 6, 9, 5, 4, -1},
		{2, 1, 6, 6, 1, 5, 8, 7, 4, -1},
		{0, 3, 7, 0, 7, 4, 2, 1, 6, 1, 5, 6, -1},
		{8, 7, 4, 6, 9, 5, 6, 0, 9, 6, 2, 0, -1},
		{7, 2, 3, 6, 2, 7, 5, 4, 9, -1},
		{4, 8, 7, 3, 6, 11, 3, 5, 6, 3, 1, 5, -1},
		{5, 0, 1, 5, 4, 0, 7, 6, 11, -1},
		{9, 5, 4, 6, 11, 7, 0, 8, 3, -1},
		{11, 7, 6, 9, 5, 4, -1},
		{6, 10, 4, 4, 10, 9, -1},
		{6, 10, 4, 4, 10, 9, 3, 8, 0, -1},
		{0, 10, 1, 0, 6, 10, 0, 4, 6, -1},
		{6, 10, 1, 6, 1, 8, 1, 3, 8, 4, 6, 8, -1},
		{9, 4, 10, 10, 4, 6, 3, 2, 11, -1},
		{2, 11, 8, 2, 8, 0, 6, 10, 4, 10, 9, 4, -1},
		{11, 3, 2, 0, 10, 1, 0, 6, 10, 0, 4, 6, -1},
		{6, 8, 4, 11, 8, 6, 2, 10, 1, -1},
		{4, 1, 9, 4, 2, 1, 4, 6, 2, -1},
		{3, 8, 0, 4, 1, 9, 4, 2, 1, 4, 6, 2, -1},
		{6, 2, 4, 4, 2, 0, -1},
		{3, 8, 2, 8, 4, 2, 4, 6, 2, -1},
		{4, 6, 9, 6, 11, 3, 9, 6, 3, 9, 3, 1, -1},
		{8, 6, 11, 4, 6, 8, 9, 0, 1, -1},
		{11, 3, 6, 3, 0, 6, 0, 4, 6, -1},
		{8, 6, 11, 4, 6, 8, -1},
		{10, 7, 6, 10, 8, 7, 10, 9, 8, -1},
		{3, 7, 0, 7, 6, 10, 0, 7, 10, 0, 10, 9, -1},
		{6, 10, 7, 7, 10, 8, 8, 10, 1, 8, 1, 0, -1},
		{6, 10, 7, 10, 1, 7, 1, 3, 7, -1},
		{3, 2, 11, 10, 7, 6, 10, 8, 7, 10, 9, 8, -1},
		{2, 9, 0, 10, 9, 2, 6, 11, 7, -1},
		{0, 8, 3, 7, 6, 11, 1, 2, 10, -1},
		{7, 6, 11, 1, 2, 10, -1},
		{2, 1, 9, 2, 9, 7, 9, 8, 7, 6, 2, 7, -1},
		{2, 7, 6, 3, 7, 2, 0, 1, 9, -1},
		{8, 7, 0, 7, 6, 0, 6, 2, 0, -1},
		{7, 2, 3, 6, 2, 7, -1},
		{8, 1, 9, 3, 1, 8, 11, 7, 6, -1},
		{11, 7, 6, 1, 9, 0, -1},
		{6, 11, 7, 0, 8, 3, -1},
		{11, 7, 6, -1},
		{7, 11, 5, 5, 11, 10, -1},
		{10, 5, 11, 11, 5, 7, 0, 3, 8, -1},
		{7, 11, 5, 5, 11, 10, 0, 9, 1, -1},
		{7, 11, 10, 7, 10, 5, 3, 8, 1, 8, 9, 1, -1},
		{5, 2, 10, 5, 3, 2, 5, 7, 3, -1},
		{5, 7, 10, 7, 8, 0, 10, 7, 0, 10, 0, 2, -1},
		{0, 9, 1, 5, 2, 10, 5, 3, 2, 5, 7, 3, -1},
		{9, 7, 8, 5, 7, 9, 10, 1, 2, -1},
		{1, 11, 2, 1, 7, 11, 1, 5, 7, -1},
		{8, 0, 3, 1, 11, 2, 1, 7, 11, 1, 5, 7, -1},
		{7, 11, 2, 7, 2, 9, 2, 0, 9, 5, 7, 9, -1},
		{7, 9, 5, 8, 9, 7, 3, 11, 2, -1},
		{3, 1, 7, 7, 1, 5, -1},
		{8, 0, 7, 0, 1, 7, 1, 5, 7, -1},
		{0, 9, 3, 9, 5, 3, 5, 7, 3, -1},
		{9, 7, 8, 5, 7, 9, -1},
		{8, 5, 4, 8, 10, 5, 8, 11, 10, -1},
		{0, 3, 11, 0, 11, 5, 11, 10, 5, 4, 0, 5, -1},
		{1, 0, 9, 8, 5, 4, 8, 10, 5, 8, 11, 10, -1},
		{10, 3, 11, 1, 3, 10, 9, 5, 4, -1},
		{3, 2, 8, 8, 2, 4, 4, 2, 10, 4, 10, 5, -1},
		{10, 5, 2, 5, 4, 2, 4, 0, 2, -1},
		{5, 4, 9, 8, 3, 0, 10, 1, 2, -1},
		{2, 10, 1, 4, 9, 5, -1},
		{8, 11, 4, 11, 2, 1, 4, 11, 1, 4, 1, 5, -1},
		{0, 5, 4, 1, 5, 0, 2, 3, 11, -1},
		{0, 11, 2, 8, 11, 0, 4, 9, 5, -1},
		{5, 4, 9, 2, 3, 11, -1},
		{4, 8, 5, 8, 3, 5, 3, 1, 5, -1},
		{0, 5, 4, 1, 5, 0, -1},
		{5, 4, 9, 3, 0, 8, -1},
		{5, 4, 9, -1},
		{11, 4, 7, 11, 9, 4, 11, 10, 9, -1},
		{0, 3, 8, 11, 4, 7, 11, 9, 4, 11, 10, 9, -1},
		{11, 10, 7, 10, 1, 0, 7, 10, 0, 7, 0, 4, -1},
		{3, 10, 1, 11, 10, 3, 7, 8, 4, -1},
		{3, 2, 10, 3, 10, 4, 10, 9, 4, 7, 3, 4, -1},
		{9, 2, 10, 0, 2, 9, 8, 4, 7, -1},
		{3, 4, 7, 0, 4, 3, 1, 2, 10, -1},
		{7, 8, 4, 10, 1, 2, -1},
		{7, 11, 4, 4, 11, 9, 9, 11, 2, 9, 2, 1, -1},
		{1, 9, 0, 4, 7, 8, 2, 3, 11, -1},
		{7, 11, 4, 11, 2, 4, 2, 0, 4, -1},
		{4, 7, 8, 2, 3, 11, -1},
		{9, 4, 1, 4, 7, 1, 7, 3, 1, -1},
		{7, 8, 4, 1, 9, 0, -1},
		{3, 4, 7, 0, 4, 3, -1},
		{7, 8, 4, -1},
		{11, 10, 8, 8, 10, 9, -1},
		{0, 3, 9, 3, 11, 9, 11, 10, 9, -1},
		{1, 0, 10, 0, 8, 10, 8, 11, 10, -1},
		{10, 3, 11, 1, 3, 10, -1},
		{3, 2, 8, 2, 10, 8, 10, 9, 8, -1},
		{9, 2, 10, 0, 2, 9, -1},
		{8, 3, 0, 10, 1, 2, -1},
		{2, 10, 1, -1},
		{2, 1, 11, 1, 9, 11, 9, 8, 11, -1},
		{11, 2, 3, 9, 0, 1, -1},
		{11, 0, 8, 2, 0, 11, -1},
		{3, 11, 2, -1},
		{1, 8, 3, 9, 8, 1, -1},
		{1, 9, 0, -1},
		{8, 3, 0, -1},
		{-1} 
    };

	// Constructor:
	// Dynamically allocates a 3D voxel grid storing
	// floats, which represent the terrain's surface
	Terrain::Terrain(int _x, int _y, int _z, float _voxel_size)
	{
		// Assign x, y, z sizes
		x_size = _x;
		y_size = _y;
		z_size = _z;

		// Create grid
		grid = new float**[x_size];

		for (int i = 0; i < x_size; i++)
		{
			grid[i] = new float*[y_size];

			for (int j = 0; j < y_size; j++)
			{
				grid[i][j] = new float[z_size];

				// Randomly generate values
				for (int k = 0; k < z_size; k++)
				{
					grid[i][j][k] = Math::Compute::random(2.5f, 10.f);
				}
			}
		}

        // Assign voxel size and threshold
        voxel_size = _voxel_size;
        surface_level = 6.5f;

        // Set outermost vertices of grid to non-surface so that the 
        // terrain is not transparent from any angle
        // X-Plane
        for (int j = 0; j < y_size; j++)
        {
            for (int k = 0; k < z_size; k++)
            {
                grid[0][j][k] = surface_level - 1;
                grid[x_size - 1][j][k] = surface_level - 1;
            }
        }

        // Y-Plane
        for (int j = 0; j < x_size; j++)
        {
            for (int k = 0; k < z_size; k++)
            {
                grid[j][0][k] = surface_level - 1;
                grid[j][y_size - 1][k] = surface_level - 1;
            }
        }

        // Z-Plane
        for (int j = 0; j < x_size; j++)
        {
            for (int k = 0; k < y_size; k++)
            {
                grid[j][k][0] = surface_level - 1;
                grid[j][k][z_size - 1] = surface_level - 1;
            }
        }

		// Generate mesh
		generateMesh();
	}

	void Terrain::checkConfiguration(int mask)
	{
		grid[0][0][0] = surface_level * ((mask >> 0) & 1);
		grid[1][0][0] = surface_level * ((mask >> 1) & 1);
		grid[0][1][0] = surface_level * ((mask >> 2) & 1);
		grid[1][1][0] = surface_level * ((mask >> 3) & 1);
		grid[0][0][1] = surface_level * ((mask >> 4) & 1);
		grid[1][0][1] = surface_level * ((mask >> 5) & 1);
		grid[0][1][1] = surface_level * ((mask >> 6) & 1);
		grid[1][1][1] = surface_level * ((mask >> 7) & 1);

		generateMesh();
	}
	// Destructor:
	// Deallocates the 3D voxel grid
	Terrain::~Terrain()
	{
		for (int i = 0; i < x_size; i++)
		{
			for (int j = 0; j < y_size; j++)
			{
				delete[] grid[i][j];
			}

			delete[] grid[i];
		}

		delete[] grid;
	}

    // Get Mesh:
    // Returns the terrain's mesh
    Mesh& Terrain::getMesh()
    {
        return mesh;
    }

	// SamplePoint:
	// Returns the floating point value at a given point
	float Terrain::samplePoint(int x, int y, int z)
	{
		return grid[x][y][z];
	}

	// GenerateMesh:
	// Generates the mesh for the grid. Must be called anytime the terrain
	// is changed.
	void Terrain::generateMesh()
	{
        // Reset mesh
        mesh = Mesh(XYZ);
        
        // For every voxel, generate the marching cube
        int index = 0;

        for (int x = 0; x < x_size - 1; x++)
        {
            for (int y = 0; y < y_size - 1; y++)
            {
                for (int z = 0; z < z_size - 1; z++)
                {
                    // Get edge mask determining what marching cube to generate
                    int mask = edgeMask(x,y,z);

                    // Get edges 
                    std::vector<int> edges = TriangleTable[mask];

                    for (int i = 0; i < edges.size(); i += 3)
                    {
						if (edges[i] != -1)
						{
							// Generate triangle
							Vector3 p1 = terrainCoordinate(x, y, z, edges[i]);
							Vector3 p2 = terrainCoordinate(x, y, z, edges[i + 1]);
							Vector3 p3 = terrainCoordinate(x, y, z, edges[i + 2]);

							assert(!(p1.x == p2.x && p1.y == p2.y && p1.z == p2.z));
							assert(!(p1.x == p3.x && p1.y == p3.y && p1.z == p3.z));
							assert(!(p3.x == p2.x && p3.y == p2.y && p3.z == p2.z));

							// Load into mesh
							float coord_1[3] = { p1.x, p1.y, p1.z };
							float coord_2[3] = { p2.x, p2.y, p2.z };
							float coord_3[3] = { p3.x, p3.y, p3.z };

							mesh.addVertex(coord_1);
							mesh.addIndex(index++);

							mesh.addVertex(coord_2);
							mesh.addIndex(index++);

							mesh.addVertex(coord_3);
							mesh.addIndex(index++);
						}
                    }

                }
            }
        }

        // Calculate mesh normals
        mesh.calculateNormals();
        
        // Set shader
        mesh.setShaders("Default", "Default");
	}

    // SampleEdgeMask:
    // Finds the edge mask at a voxel given at (x,y,z) coordinates, determining what
    // marching cube to generate.
    // Points of the cube above the given threshold are considered to be the terrain.
    int Terrain::edgeMask(int x, int y, int z)
    {
        int mask = 0;

        mask |= (grid[x][y][z] >= surface_level) << 0;
        mask |= (grid[x + 1][y][z] >= surface_level) << 1;
        mask |= (grid[x][y + 1][z] >= surface_level) << 2;
        mask |= (grid[x + 1][y + 1][z] >= surface_level) << 3;
        mask |= (grid[x][y][z + 1] >= surface_level) << 4;
        mask |= (grid[x + 1][y][z + 1] >= surface_level) << 5;
        mask |= (grid[x][y + 1][z + 1] >= surface_level) << 6;
        mask |= (grid[x + 1][y + 1][z + 1] >= surface_level) << 7;

        return mask;
    }

    // TerrainCoordinate:
    // Determines the coordinate of the terrain given at some edge_ID
    // for voxel at (x,y,z)
    Vector3 Terrain::terrainCoordinate(int x, int y, int z, int edge_ID)
    {
        Vector3 coords; 

        // We have 2 points that form an edge - we sample the data at these points
        Vector3 base_point;
        float base_value = 0;

        Vector3 offset;
        float offset_value = 0;
        
        // Based on the edge, determine the location of the edge's points,
        // and sample at these edge points
        switch (edge_ID)
        {
        case 0:
        {
            base_point = Vector3(x, y, z);
            base_value = grid[x][y][z];
            offset = Vector3(1, 0, 0);
            offset_value = grid[x + 1][y][z];
        }
        break;

        case 1:
        {
            base_point = Vector3(x + 1, y, z);
            base_value = grid[x + 1][y][z];
            offset = Vector3(0, 1, 0);
            offset_value = grid[x + 1][y + 1][z];
        }
        break;
            

        case 2:
        {
            base_point = Vector3(x + 1, y + 1, z);
            base_value = grid[x + 1][y + 1][z];
            offset = Vector3(-1, 0, 0);
            offset_value = grid[x][y + 1][z];
        }
        break;

        case 3:
        {
            base_point = Vector3(x, y + 1, z);
            base_value = grid[x][y + 1][z];
            offset = Vector3(0, -1, 0);
            offset_value = grid[x][y][z];
        }
        break;

        case 4:
        {
            base_point = Vector3(x, y, z + 1);
            base_value = grid[x][y][z + 1];
            offset = Vector3(1, 0, 0);
            offset_value = grid[x + 1][y][z + 1];
        }
        break;


        case 5:
        {
            base_point = Vector3(x + 1, y, z + 1);
            base_value = grid[x + 1][y][z + 1];
            offset = Vector3(0, 1, 0);
            offset_value = grid[x + 1][y + 1][z + 1];
        }
        break;


        case 6:
        {
            base_point = Vector3(x + 1, y + 1, z + 1);
            base_value = grid[x + 1][y + 1][z + 1];
            offset = Vector3(-1, 0, 0);
            offset_value = grid[x][y + 1][z + 1];
        }
        break;


        case 7:
        {
            base_point = Vector3(x, y + 1, z + 1);
            base_value = grid[x][y + 1][z + 1];
            offset = Vector3(0, -1, 0);
            offset_value = grid[x][y][z + 1];
        }
        break;


        case 8:
        {
            base_point = Vector3(x, y, z);
            base_value = grid[x][y][z];
            offset = Vector3(0, 0, 1);
            offset_value = grid[x][y][z + 1];
        }
        break;


        case 9:
        {
			base_point = Vector3(x + 1, y, z);
			base_value = grid[x + 1][y][z];
			offset = Vector3(0, 0, 1);
			offset_value = grid[x + 1][y][z + 1];
        }
        break;

        case 10:
        {
            base_point = Vector3(x + 1, y + 1, z);
            base_value = grid[x + 1][y + 1][z];
            offset = Vector3(0, 0, 1);
            offset_value = grid[x + 1][y + 1][z + 1];
        }
        break;

        case 11:
        {
            base_point = Vector3(x, y + 1, z);
            base_value = grid[x][y + 1][z];
            offset = Vector3(0, 0, 1);
            offset_value = grid[x][y + 1][z + 1];
        }
        break;
        }

        // Linearly interpolate the location of the surface point 
        // based on the sampled values
		float offset_percent = 0.5f;
		//float offset_percent = (surface_level - base_value) / (offset_value - base_value);
		// 
		assert(offset_percent != 0);
		// 
        // Determine surface coordinate
        coords = (base_point + (offset * offset_percent)) * voxel_size;
        
        return coords;
    }

	*/
}
}