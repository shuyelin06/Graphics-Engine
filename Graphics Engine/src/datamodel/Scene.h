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
	friend class Engine::Graphics::VisualEngine;

	private:
		std::vector<Camera> cameras;
		std::vector<Light> lights;
		std::vector<Object> objects;

	public:
		// Constructors
		Scene();

		// Scene Creation Methods
		Camera* createCamera();	// Create a new camera
		Object* createObject(); // Create a new object
		Light* createLight();	// Create a new light

	
	};
} 
}