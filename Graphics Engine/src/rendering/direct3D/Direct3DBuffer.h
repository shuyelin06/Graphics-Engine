#pragma once

#include "Direct3D11.h"

#include "rendering/core/Buffer.h"

namespace Engine
{

namespace Graphics
{
	// Direct3DBuffer
	// Represents a buffer class which
	// will be used in the Direct3D graphics API
	class Direct3DBuffer : public Buffer
	{
	private:
		ID3D11Buffer* buffer;

	public:
		Direct3DBuffer(ID3D11Device* device, void* data, int size);
		
		ID3D11Buffer** getBufferAddress();
		ID3D11Buffer* getBuffer();
	};
}
}