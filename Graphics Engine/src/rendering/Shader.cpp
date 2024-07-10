#include "Shader.h"

#include <cstring>
#include <assert.h>

namespace Engine
{
namespace Graphics
{
	CBHandle::CBHandle() :
		format(), formatIndex(0),
		data(), resource(nullptr) {}

	int CBHandle::byteSize() const
	{
		assert(sizeof(float) == 4);

		int numFloats = 0;

		for (CBDataFormat format : format)
		{
			// Verify that alignment is being met. If not, add necessary
			// padding until met.
			if (numFloats % 4 != 0 && (numFloats % 4) + format > 4)
				numFloats += 4 - (numFloats % 4);

			numFloats += format;
		}

		return numFloats * sizeof(float);
	}

	Shader::Shader() = default; 
	Shader::~Shader() = default;

	void Shader::enableCB(CBSlot slot, const CBDataFormat formatDescription[], int numberDescriptors)
	{
		assert(activeConstantBuffers[slot] == false);

		CBHandle& constantBuffer = constantBuffers[slot];

		for (int i = 0; i < numberDescriptors; i++)
			constantBuffer.format.push_back(formatDescription[i]);

		activeConstantBuffers[slot] = true;
	}

	void Shader::addCBData(CBSlot slot, void* data, CBDataFormat format)
	{
		CBHandle& constantBuffer = constantBuffers[slot];
		
		// Validate that this is the correct data format to pass into the constant buffer
		assert(sizeof(float) == 4);
		assert(format % 4 == 0);

		assert(constantBuffer.formatIndex < constantBuffer.format.size());
		assert(constantBuffer.format[constantBuffer.formatIndex] == format);

		// If the format will not fit into the 16-byte alignment, add padding.
		// Note that floats are 4 bytes by the IEEE standard.
		std::vector<float>& cbData = constantBuffer.data;
		const int numFloats = format / 4;

		if (cbData.size() % 4 != 0 && (cbData.size() % 4) + numFloats > 4)
			cbData.resize(cbData.size() + 4 - (cbData.size() % 4));

		// Now, copy our data into the data vector.
		float* floatData = static_cast<float*>(data);
		float value;

		for (int i = 0; i < numFloats; i++)
		{	
			std::memcpy(&value, floatData + i, sizeof(float));
			cbData.push_back(value);
		}

		constantBuffer.formatIndex++;
	}

	void Shader::clearCBData(CBSlot slot)
	{
		CBHandle& constantBuffer = constantBuffers[slot];

		constantBuffer.formatIndex = 0;
		constantBuffer.data.clear();
	}

	void Shader::updateCBResource(CBSlot slot, ID3D11Device* device, ID3D11DeviceContext* context)
	{
		CBHandle& constantBuffer = constantBuffers[slot];

		// If the buffer resource has never been created before, create one
		if (constantBuffer.resource == nullptr)
		{
			// Create buffer to allow dynamic usage, i.e. accessible by
			// GPU read and CPU write. We opt for this usage so that we can update 
			// the resource on the fly when needed.
			D3D11_BUFFER_DESC buff_desc = {};
			buff_desc.ByteWidth = constantBuffer.byteSize();
			buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			buff_desc.Usage = D3D11_USAGE_DYNAMIC;
			buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			// Allocate resources
			D3D11_SUBRESOURCE_DATA sr_data;
			sr_data.pSysMem = constantBuffer.data.data();
			sr_data.SysMemPitch = 0;
			sr_data.SysMemSlicePitch = 0;

			// Create buffer
			device->CreateBuffer(&buff_desc, &sr_data, &(constantBuffer.resource));
		}
		// If buffer exists, perform resource renaming to update buffer data 
		// instead of creating a new buffer
		else
		{
			// Disable GPU access to data and obtain the my constant buffer resource
			D3D11_MAPPED_SUBRESOURCE mapped_resource = { 0 };
			context->Map(constantBuffer.resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);

			// Update the data in the resource 
			memcpy(mapped_resource.pData, constantBuffer.data.data(), constantBuffer.byteSize());

			// Reenable GPU access to data
			context->Unmap(constantBuffer.resource, 0);
		}
	}

	void VertexShader::bindShader(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		// Bind shader to the pipeline
		context->VSSetShader(shader, NULL, 0);

		// Update buffers resources, and bind them to the pipeline
		for (int i = 0; i < CBSlot::COUNT; i++)
		{
			if (activeConstantBuffers[i])
				updateCBResource((CBSlot) i, device, context);

			context->VSSetConstantBuffers(i, 1, &(constantBuffers[i].resource));
		}
	}

	void PixelShader::bindShader(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		// Bind shader to the pipeline
		context->PSSetShader(shader, NULL, 0);

		// Update buffers resources, and bind them to the pipeline
		for (int i = 0; i < CBSlot::COUNT; i++)
		{
			if (activeConstantBuffers[i])
				updateCBResource((CBSlot)i, device, context);

			context->PSSetConstantBuffers(i, 1, &(constantBuffers[i].resource));
		}
	}

}
}