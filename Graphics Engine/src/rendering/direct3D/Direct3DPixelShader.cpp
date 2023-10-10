#include "Direct3DPixelShader.h"

#include <assert.h>

namespace Engine
{

namespace Graphics
{
	// Constructor
	Direct3DPixelShader::Direct3DPixelShader(ID3D11Device* device, const wchar_t* shader_file, const char* entrypoint)
	{
		pixel_shader = nullptr;

		// Compiler settings
		ID3DInclude* include_settings = D3D_COMPILE_STANDARD_FILE_INCLUDE;
		const char* compiler_target = "ps_5_0";
		const UINT flags = 0 | D3DCOMPILE_ENABLE_STRICTNESS;

		// Compile blob
		ID3DBlob* error_blob = NULL; // Tracks if errors occurred
		ID3DBlob* compile_blob = NULL; // Blob we're compiling to

		HRESULT result;
		result = D3DCompileFromFile(
			shader_file,
			nullptr,
			include_settings,
			entrypoint,
			compiler_target,
			flags,
			0,
			&compile_blob,
			&error_blob
		);


		// Verify success
		if (FAILED(result))
		{
			// Print Error
			if (error_blob)
			{
				OutputDebugStringA((char*)error_blob->GetBufferPointer());
				error_blob->Release();
			}
			// Release Pixel Shader Blob
			if (compile_blob) { compile_blob->Release(); }

			assert(false);
		}

		// Create shader
		device->CreatePixelShader(
			compile_blob->GetBufferPointer(),
			compile_blob->GetBufferSize(),
			NULL,
			&pixel_shader
		);
	}

	// Returns the Direct3D shader for use in the renderer
	ID3D11PixelShader* Direct3DPixelShader::getShader()
	{
		return pixel_shader;
	}

}
}