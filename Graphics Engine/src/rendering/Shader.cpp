#include "Shader.h"

#include <cstring>
#include <assert.h>

namespace Engine
{
namespace Graphics
{
	CBHandle::CBHandle(const CBDataFormat formatDescription[], int numberDescriptors) :
		format(), formatIndex(0), data(), resource(nullptr) 
	{
		for (int i = 0; i < numberDescriptors; i++)
			format.push_back(formatDescription[i]);
	}
	CBHandle::~CBHandle() = default;

	void CBHandle::loadData(const void* dataPtr, CBDataFormat dataFormat)
	{
		// Validate that this is the correct data format to pass into the constant buffer
		assert(sizeof(float) == 4);
		assert(dataFormat % 4 == 0);

		assert(formatIndex < format.size());
		assert(format[formatIndex] == dataFormat);

		// If the format will not fit into the 16-byte alignment, add padding.
		// Note that floats are 4 bytes by the IEEE standard.
		const int numFloats = dataFormat / 4;

		if (data.size() % 4 != 0 && (data.size() % 4) + numFloats > 4)
			data.resize(data.size() + 4 - (data.size() % 4));

		// Now, copy our data into the data vector.
		const float* floatData = static_cast<const float*>(dataPtr);
		float value;

		for (int i = 0; i < numFloats; i++)
		{
			std::memcpy(&value, floatData + i, sizeof(float));
			data.push_back(value);
		}

		formatIndex++;
	}

	void CBHandle::clearData()
	{
		formatIndex = 0;
		data.clear();
	}

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

	Shader::Shader()
	{
		for (int i = 0; i < CBSlot::CBCOUNT; i++)
			constantBuffers[i] = nullptr;
	}
	Shader::~Shader()
	{
		for (int i = 0; i < CBSlot::CBCOUNT; i++)
			delete constantBuffers[i];
	}

	void Shader::enableCB(CBSlot slot, const CBDataFormat formatDescription[], int numberDescriptors)
	{
		assert(constantBuffers[slot] == nullptr);
		constantBuffers[slot] = new CBHandle(formatDescription, numberDescriptors);
	}

	CBHandle* Shader::getCBHandle(CBSlot slot)
	{
		assert(constantBuffers[slot] != nullptr);
		return constantBuffers[slot];
	}

	void Shader::updateCBResource(CBSlot slot, ID3D11Device* device, ID3D11DeviceContext* context)
	{
		CBHandle* constantBuffer = constantBuffers[slot];
		assert(constantBuffer != nullptr);
		
		// If the buffer resource has never been created before, create one
		if (constantBuffer->resource == nullptr)
		{
			// Create buffer to allow dynamic usage, i.e. accessible by
			// GPU read and CPU write. We opt for this usage so that we can update 
			// the resource on the fly when needed.
			D3D11_BUFFER_DESC buff_desc = {};
			buff_desc.ByteWidth = constantBuffer->byteSize();
			buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			buff_desc.Usage = D3D11_USAGE_DYNAMIC;
			buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			// Allocate resources
			D3D11_SUBRESOURCE_DATA sr_data;
			sr_data.pSysMem = constantBuffer->data.data();
			sr_data.SysMemPitch = 0;
			sr_data.SysMemSlicePitch = 0;

			// Create buffer
			device->CreateBuffer(&buff_desc, &sr_data, &(constantBuffer->resource));
		}
		// If buffer exists, perform resource renaming to update buffer data 
		// instead of creating a new buffer
		else
		{
			// Disable GPU access to data and obtain the my constant buffer resource
			D3D11_MAPPED_SUBRESOURCE mapped_resource = { 0 };
			context->Map(constantBuffer->resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);

			// Update the data in the resource 
			memcpy(mapped_resource.pData, constantBuffer->data.data(), constantBuffer->byteSize());

			// Reenable GPU access to data
			context->Unmap(constantBuffer->resource, 0);
		}
	}

	VertexShader::VertexShader(ID3D11VertexShader* _shader, ID3D11InputLayout* _layout)
	{
		shader = _shader;
		layout = _layout;
	}
	VertexShader::~VertexShader() = default;

	void VertexShader::bindShader(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		// Bind input layout to the pipeline
		context->IASetInputLayout(layout);

		// Bind shader to the pipeline
		context->VSSetShader(shader, NULL, 0);

		// Update buffers resources, and bind them to the pipeline
		for (int i = 0; i < CBSlot::CBCOUNT; i++)
		{
			if (constantBuffers[i] != nullptr)
			{
				updateCBResource((CBSlot)i, device, context);
				context->VSSetConstantBuffers(i, 1, &(constantBuffers[i]->resource));
			}
		}
	}

	PixelShader::PixelShader(ID3D11PixelShader* _shader)
	{
		shader = _shader;
	}
	PixelShader::~PixelShader() = default;

	void PixelShader::bindShader(ID3D11Device* device, ID3D11DeviceContext* context)
	{
		// Bind shader to the pipeline
		context->PSSetShader(shader, NULL, 0);

		// Update buffers resources, and bind them to the pipeline
		for (int i = 0; i < CBSlot::CBCOUNT; i++)
		{
			if (constantBuffers[i] != nullptr)
			{
				updateCBResource((CBSlot)i, device, context);
				context->PSSetConstantBuffers(i, 1, &(constantBuffers[i]->resource));
			}
		}
	}

}
}