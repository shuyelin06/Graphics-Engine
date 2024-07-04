#pragma once

#include <map>
#include <string>

#include "Mesh.h"

namespace Engine
{
namespace Graphics
{
	// MeshManager Class:
	// Manages meshes for the engine. Provides methods
	// to load meshes, view them, and manipulate them.
	class MeshManager
	{
	private:
		std::map<std::string, Mesh*> meshes;

	public:
		MeshManager();
		~MeshManager();

		// Mesh Accessing
		Mesh* getMesh(std::string mesh_name);

		// Mesh Loading 
		void LoadMeshFromOBJ(std::string obj_file, std::string mesh_name);
	};

}
}