#pragma once

#include "objects/PhysicsObject.h"

#include "Camera.h"

namespace Engine
{

namespace Datamodel
{

	class Player : public PhysicsObject
	{
	private:
		Camera camera;

	public:
		Player();

		Camera* getCamera();
	};
}
}