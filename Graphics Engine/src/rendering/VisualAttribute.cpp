#include "VisualAttribute.h"

namespace Engine
{
namespace Graphics
{
	// Default Constructor:
	// Assigns the object reference
	VisualAttribute::VisualAttribute(Object* _object)
	{
		object = _object;
	}
	
	// Destructor:
	// Destructs the VisualAttribute
	VisualAttribute::~VisualAttribute()
	{

	}

	// Prepare:
	// Loads the object's local_to_world matrix
	void VisualAttribute::prepare()
	{
		
	}

}
}