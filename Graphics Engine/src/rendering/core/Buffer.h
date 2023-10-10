#pragma once

namespace Engine
{

namespace Graphics
{
	// Represents a buffer class, which will store data
	// which can either be used as vertex data or constant
	// buffer data
	class Buffer
	{
	};

	class VertexBuffer : public Buffer
	{
	};

	class ConstantBuffer : public Buffer
	{
	};

}
}