#include "Direct3DVertexShader.h"

#include <assert.h>

namespace Engine
{

namespace Graphics
{
	// Constructor: Creates a vertex shader
	Direct3DVertexShader::Direct3DVertexShader(ID3D11Device* device, const wchar_t* shader_file, const char* entrypoint, std::vector<InputLayout>& layout)
	{
		vertex_shader = nullptr;
		input_layout = nullptr;

		// Compiler settings
		ID3DInclude* include_settings = D3D_COMPILE_STANDARD_FILE_INCLUDE;
		const char* compiler_target = "vs_5_0";
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

		// Create input layout
		D3D11_INPUT_ELEMENT_DESC* input_description = new D3D11_INPUT_ELEMENT_DESC[layout.size()];
		int description_size = layout.size();

		for (int i = 0; i < layout.size(); i++)
		{
			D3D11_INPUT_ELEMENT_DESC desc;

			switch (layout[i])
			{
			case Position3:
				desc = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
			}

			input_description[i] = desc;
		}

		device->CreateInputLayout(
			input_description,
			description_size,
			compile_blob->GetBufferPointer(),
			compile_blob->GetBufferSize(),
			&input_layout
		);

		// Create shader
		device->CreateVertexShader(
			compile_blob->GetBufferPointer(),
			compile_blob->GetBufferSize(),
			NULL,
			&vertex_shader
		);

	}

	// Returns the Direct3D shader for use in the renderer
	ID3D11VertexShader* Direct3DVertexShader::getShader()
	{
		return vertex_shader; 
	}

	ID3D11InputLayout* Direct3DVertexShader::getInputLayout()
	{
		return input_layout;
	}

}
}