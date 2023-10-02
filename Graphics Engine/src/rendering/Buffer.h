#pragma once

#include "Direct3D11.h"

namespace Engine
{

namespace Graphics 
{

	// Buffer Class
	// Defines the class for a buffer, which can be
	// bound to a vertex or pixel shader
	class Buffer
	{
	public:
		ID3D11Buffer* buffer_ptr;

		Buffer();

		void initialize(int byte_size, void* data);
	};

} // Namespace Graphics
} // Namespace Engine