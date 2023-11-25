#include "Shaders.h"

#include <assert.h>

namespace Engine
{
	/* Shader Class Declarations */
	Shader::Shader(ShaderType _type)
	{
		type = _type;
		shader_ptr = NULL;
	}

	// Returns the shader pointer
	void* Shader::getShader() { return shader_ptr; }

	// Compiles a blob from a shader file + entrypoint
	void Shader::compileBlob(const wchar_t* file, const char* entry)
	{
		// Shader must have a type assigned
		assert(type != None);

		// Compiler settings
		ID3DInclude* include_settings = D3D_COMPILE_STANDARD_FILE_INCLUDE;
		const char* compiler_target = (type == Vertex) ? "vs_5_0" : "ps_5_0";
		const UINT flags = 0 | D3DCOMPILE_ENABLE_STRICTNESS;

		// Compile blob
		ID3DBlob* error_blob = NULL; // Tracks if errors occurred
		ID3DBlob* compile_blob = NULL; // Blob we're compiling to
		
		HRESULT result;
		result = D3DCompileFromFile(
			file,
			nullptr,
			include_settings,
			entry,
			compiler_target,
			flags,
			0,
			&compile_blob,
			&error_blob
		);

		shader_ptr = compile_blob;

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
			if (shader_ptr) { static_cast<ID3DBlob*>(shader_ptr)->Release(); }

			assert(false);
		}

	}
	
	// Does nothing, as the shader creation will be inherited
	// and implemented by children classes
	void Shader::createShader(ID3D11Device* device)
	{
		assert(false);
	}

	/* PixelShader Class Definition */
	PixelShader::PixelShader() : Shader(Pixel) {}

	void PixelShader::createShader(ID3D11Device* device)
	{
		ID3DBlob* shader_blob = static_cast<ID3DBlob*>(shader_ptr);
		ID3D11PixelShader* pixel_shader = NULL;

		device->CreatePixelShader(
			shader_blob->GetBufferPointer(),
			shader_blob->GetBufferSize(),
			NULL,
			&pixel_shader
		);

		shader_ptr = pixel_shader;
	}

	/* VertexShader Class Definition */
	VertexShader::VertexShader(InputLayoutDescription* desc) : Shader(Vertex)
	{
		input_layout_ptr = desc;
	}

	VertexShader::VertexShader() : Shader(Vertex)
	{
		input_layout_ptr = NULL;
	}

	ID3D11InputLayout* VertexShader::getInputLayout() 
	{
		return static_cast<ID3D11InputLayout*>(input_layout_ptr); 
	}

	void VertexShader::createShader(ID3D11Device* device)
	{
		ID3DBlob* shader_blob = static_cast<ID3DBlob*>(shader_ptr);
		InputLayoutDescription* input_desc = static_cast<InputLayoutDescription*>(input_layout_ptr);

		// Create input layout
		ID3D11InputLayout* input_layout;

		device->CreateInputLayout(
			input_desc->description,
			input_desc->size,
			shader_blob->GetBufferPointer(),
			shader_blob->GetBufferSize(),
			&input_layout
		);

		input_layout_ptr = input_layout;

		// Create shader
		ID3D11VertexShader* vertex_shader = NULL;

		device->CreateVertexShader(
			shader_blob->GetBufferPointer(),
			shader_blob->GetBufferSize(),
			NULL,
			&vertex_shader
		);

		shader_ptr = vertex_shader;
	}

}