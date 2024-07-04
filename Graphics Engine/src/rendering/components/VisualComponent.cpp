#include "VisualComponent.h"

namespace Engine
{
using namespace Datamodel;
namespace Graphics
{
	// Constructor:
	// Saves a reference to the visual system
	VisualComponent::VisualComponent(Object* object, VisualSystem* _system)
		: Component(object)
	{
		system = _system;
	}
	
}
}