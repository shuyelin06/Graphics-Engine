#include "Scene.h"

namespace Engine
{
namespace Datamodel
{
	// Default Constructor:
	// Creates an empty scene
	Scene::Scene()
	{
		camera = Camera();
		objects.clear();
		lights.clear();
	}

	// GetCamera:
	// Returns the scene's camera
	Camera& Scene::getCamera()
	{
		return camera;
	}

	// GetObjects:
	// Returns the scene's vector of objects
	vector<Object>& Scene::getObjects()
	{
		return objects;
	}

	// GetLights:
	// Returns the scene's vector of lights
	vector<Light>& Scene::getLights()
	{
		return lights;
	}

	// CreateObject:
	// Creates an empty object within the scene,
	// and returns a pointer to it
	Object& Scene::createObject()
	{
		int index = objects.size();
		objects.push_back(Object());
		return objects[index];
	}

	// CreateLight:
	// Creates an empty light within the scene,
	// and returns a pointer to it
	Light& Scene::createLight()
	{
		int index = lights.size();
		lights.push_back(Light());
		return lights[index];
	}
}
}