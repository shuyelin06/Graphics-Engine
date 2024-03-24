#include "Scene.h"

namespace Engine
{
namespace Datamodel
{
	// Default Constructor:
	// Creates an empty scene
	Scene::Scene()
	{
		objects.clear();
		lights.clear();
	}

	// CreateCamera:
	// Creates a camera within the scene,
	// and returns a pointer to it
	Camera* Scene::createCamera()
	{
		int index = cameras.size();
		cameras.push_back(Camera());
		return &(cameras[index]);
	}

	// CreateObject:
	// Creates an empty object within the scene,
	// and returns a pointer to it
	Object* Scene::createObject()
	{
		int index = objects.size();
		objects.push_back(Object());
		return &(objects[index]);
	}

	// CreateLight:
	// Creates an empty light within the scene,
	// and returns a pointer to it
	Light* Scene::createLight()
	{
		int index = lights.size();
		lights.push_back(Light());
		return &(lights[index]);
	}
}
}