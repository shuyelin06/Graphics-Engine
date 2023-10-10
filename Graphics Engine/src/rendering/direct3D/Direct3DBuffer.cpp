#include "Direct3DBuffer.h"

#include <assert.h>

namespace Engine
{

namespace Graphics
{
	// Constructor
	Direct3DBuffer::Direct3DBuffer(ID3D11Device* device, void* data, int size)
	{
		// Fill buffer description
		D3D11_BUFFER_DESC description = { 0 };
		description.ByteWidth = size;
		description.Usage = D3D11_USAGE_DEFAULT;
		// D3D11_BIND_CONSTANT_BUFFER | 
		description.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		// Fill subresource data
		D3D11_SUBRESOURCE_DATA init_data;
		init_data.pSysMem = data;
		init_data.SysMemPitch = 0;
		init_data.SysMemSlicePitch = 0;

		// Create buffer
		HRESULT hr = device->CreateBuffer(&description, &init_data, &buffer);

		assert(SUCCEEDED(hr));
	}	
	
	// Returns a buffer to be used by the Direct3DRenderer
	ID3D11Buffer** Direct3DBuffer::getBufferAddress()
	{
		return &buffer;
	}
	
	ID3D11Buffer* Direct3DBuffer::getBuffer()
	{
		return buffer;
	}
}
}