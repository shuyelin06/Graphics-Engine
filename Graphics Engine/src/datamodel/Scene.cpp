#include "Scene.h"

#include "math/Compute.h"

namespace Engine
{
namespace Datamodel
{
	// Default Constructor:
	// Creates an empty scene
	Scene::Scene() : terrain(15, 15, 15, 7.5f)
	{
		camera = Camera();

		objects.clear();
		lights.clear();
	}

	// Destructor:
	// Frees all memory allocated within the scene
	Scene::~Scene()
	{
		for (Object* object : objects)
			delete object;

		for (Light* light : lights)
			delete light;
	}

	// GetCamera:
	// Returns the scene's camera
	Camera& Scene::getCamera()
	{
		return camera;
	}

	// GetTerrain:
	// Returns the scene's terrain
	Terrain& Scene::getTerrain()
	{
		return terrain;
	}

	// GetObjects:
	// Returns the scene's vector of objects
	vector<Object*>& Scene::getObjects()
	{
		return objects;
	}

	// GetLights:
	// Returns the scene's vector of lights
	vector<Light*>& Scene::getLights()
	{
		return lights;
	}

	// CreateObject:
	// Creates an empty object within the scene,
	// and returns a pointer to it
	Object& Scene::createObject()
	{
		int index = objects.size();
		objects.push_back(new Object());
		return *(objects[index]);
	}

	// CreateLight:
	// Creates an empty light within the scene,
	// and returns a pointer to it
	Light& Scene::createLight()
	{
		Light* new_light = new Light();

		objects.push_back(new_light);
		lights.push_back(new_light);

		return *new_light;
	}

	// ClearObjects:
	// Clears all objects in the scene
	void Scene::clearObjects()
	{
		objects.clear();
	}
}
}