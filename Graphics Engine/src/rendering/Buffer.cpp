#include "Buffer.h"

namespace Engine
{

namespace Graphics
{
	/* --- Constructor --- */
	// Constructor:
	// Initializes an empty buffer instance
	Buffer::Buffer()
	{
		buffer_ptr = nullptr;
	}

	/* --- Operations --- */
	// Initialize:
	// Creates and links a buffer to the Buffer 
	// object, to be bound later to a shader
	void Buffer::initialize(int byte_size, void* data)
	{
		// Populate buffer description
		D3D11_BUFFER_DESC buffer_description;
		buffer_description.ByteWidth = byte_size;
		buffer_description.Usage = D3D11_USAGE_DYNAMIC;
		buffer_description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_description.MiscFlags = 0;
		buffer_description.StructureByteStride = 0;

		// Fill resource data
		D3D11_SUBRESOURCE_DATA buffer_data;
		buffer_data.pSysMem = data;
		buffer_data.SysMemPitch = 0;
		buffer_data.SysMemSlicePitch = 0;
	}
}
}