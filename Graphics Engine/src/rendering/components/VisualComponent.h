#pragma once

#include "datamodel/Component.h"

namespace Engine
{
namespace Graphics
{
	class VisualSystem;

	// VisualComponent Class:
	// Represents a component that can be used by the VisualSystem.
	class VisualComponent : public Datamodel::Component
	{
	protected:
		VisualSystem* system;
	
	public:
		VisualComponent(Datamodel::Object* object, VisualSystem* system);
	};
}
}