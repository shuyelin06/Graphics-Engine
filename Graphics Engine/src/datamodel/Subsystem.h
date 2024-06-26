#pragma once

namespace Engine
{
namespace Datamodel
{
	// Subsystem Class:
	// Contains generic methods that a subsystem must inherit.
	// Systems may also inherit from the ComponentHandler to support
	// the creation and destruction of components
	class Subsystem
	{
	public:
		// Initializes the subsystem
		virtual void initialize() = 0;

		// Updates the subsystem
		virtual void update() = 0;
	};
}
}