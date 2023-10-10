#pragma once

#include "Direct3D11.h"

#include "rendering/core/PixelShader.h"

namespace Engine
{

namespace Graphics
{
	// Direct3DPixelShader
	// Pixel shader to be used in the Direct3D
	// graphics api
	class Direct3DPixelShader : public PixelShader
	{
	private:
		ID3D11PixelShader* pixel_shader;

	public:
		Direct3DPixelShader(ID3D11Device* device, const wchar_t* shader_file, const char* entrypoint);

		ID3D11PixelShader* getShader();
	};
}
}