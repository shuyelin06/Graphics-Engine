#pragma once

#include "datamodel/PhysicsObject.h"

#include "datamodel/Camera.h"

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