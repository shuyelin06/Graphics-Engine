#pragma once

#include <vector>

#include "Camera.h"
#include "Object.h"
#include "Light.h"

using namespace std;

namespace Engine
{
namespace Datamodel
{
	// Scene Class:
	// Contains all of the data that represents a scene,
	// including objects and lights.
	// Various engines can interact with this scene.
	class Scene
	{
	private:
		Camera camera;
		vector<Light> lights;
		vector<Object> objects;

	public:
		// Constructors
		Scene();

		// Accessors
		Camera& getCamera();
		vector<Object>& getObjects();
		vector<Light>& getLights();

		// Create Objects in the Scene
		Object& createObject(); 
		Light& createLight();
	};
} 
}