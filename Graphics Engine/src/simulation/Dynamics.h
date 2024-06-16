#pragma once

#include "math/Vector3.h"

namespace Engine
{
namespace Simulation
{
	// Dynamics Class:
	// Represents all simulation characteristics, 
	// including physics..
	class Dynamics
	{
	private:
		Math::Vector3 velocity;
		Math::Vector3 acceleration;

	public:
		Dynamics();
	};
}
}