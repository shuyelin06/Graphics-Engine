#pragma once

#include <vector>
#include <bitset>

#include "Direct3D11.h"

namespace Engine
{
namespace Graphics
{
	// Constant Buffers:
	// Constant buffers are arrays of data which can be bound to a shader,
	// and cannot be changed for that invocation of the program.
	// Handling of any given constant buffer for a shader will be done by
	// a corresponding CBHandle object.
	enum CBSlot {
		CB0 = 0, CB1 = 1, CB2 = 2, CB3 = 3,
		COUNT // Enum trick to track the # of slots supported
	};

	enum CBDataFormat
	{
		FLOAT = 4, FLOAT2 = 8,
		FLOAT3 = 12, FLOAT4 = 16,
		FLOAT4X4 = 64
	};
	
	struct CBHandle
	{
		std::vector<CBDataFormat> format;
		int formatIndex;
		std::vector<float> data;
		ID3D11Buffer* resource;

		CBHandle();

		// Returns the number of bytes the constant buffer is 
		// expected to store (accounting for padding)
		int byteSize() const;
	};

	// Shaders:
	// Shaders are programs that can be invoked on the GPU. Currently,
	// the engine supports the following shaders:
	//    - Vertex Shader
	//    - Pixel Shader
	// Shaders can be bound to the graphics pipeline, and
	// can have data passed into their constant buffers.
	class Shader
	{
	protected:
		CBHandle constantBuffers[4];
		std::bitset<CBSlot::COUNT> activeConstantBuffers;

	public:
		Shader();
		~Shader();

		// Constant Buffer Management
		void enableCB(CBSlot slot, const CBDataFormat formatDescription[], int numberDescriptors);
		void addCBData(CBSlot slot, void* data, CBDataFormat format);
		void clearCBData(CBSlot slot);

		// Pipeline Management
		virtual void bindShader(ID3D11Device* device, ID3D11DeviceContext* context) = 0;

	protected:
		// Updates the GPU resource for the constant buffer
		void updateCBResource(CBSlot slot, ID3D11Device* device, ID3D11DeviceContext* context);
	};

	class VertexShader : public Shader
	{
	private:
		ID3D11VertexShader* shader;

	public:
		void bindShader(ID3D11Device* device, ID3D11DeviceContext* context) override;
	};

	class PixelShader : public Shader
	{
	private:
		ID3D11PixelShader* shader;

	public:
		void bindShader(ID3D11Device* device, ID3D11DeviceContext* context) override;
	};

	

	
}
}