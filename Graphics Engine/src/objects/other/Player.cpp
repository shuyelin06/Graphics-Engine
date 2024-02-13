#include "Player.h"

namespace Engine
{

namespace Datamodel
{

	Player::Player() : camera(1.2f) 
	{
		camera.setParent(this);
	};

	Camera* Player::getCamera()
	{
		return &camera;
	}

}
}