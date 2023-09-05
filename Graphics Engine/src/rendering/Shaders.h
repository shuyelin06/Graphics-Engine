#pragma once

// Direct 3D 11 Library Includes
#include <d3d11.h> // Direct 3D Interface
#include <dxgi.h> // DirectX Driver Interface
#include <d3dcompiler.h> // Shader Compiler

// Indicates Visual C++ to leave a command in the object file, which can be read by
// the linker when it processes object files.
// Tells the linker to add the "library" library to the list of library dependencies
#pragma comment( lib, "user32" )          // link against the win32 library
#pragma comment( lib, "d3d11.lib" )       // direct3D library
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler

namespace Engine
{
	typedef enum ShaderType { 
		None, Vertex, Pixel 
	} ShaderType;

	// Shader Class
	class Shader
	{
	protected:
		ShaderType type; // Stores the shader's type
		void* shader_ptr; // Will point to either the blob or the shader

	public:
		Shader(ShaderType _type);

		// Returns the shader pointer
		void* getShader();

		// Shader creation calls
		void compileBlob(const wchar_t* file, const char* entry);
		void createShader(ID3D11Device* device);
	};
	
	// PixelShader Class
	class PixelShader : public Shader
	{
	public:
		PixelShader();

		void createShader(ID3D11Device* device);
	};

	// Input Layout
	struct InputLayoutDescription
	{
		D3D11_INPUT_ELEMENT_DESC* description;
		u_int size;
	};

	// VertexShader Class
	class VertexShader : public Shader
	{
	protected:
		void* input_layout_ptr; // Stores either the input layout, or its description

	public:
		VertexShader();
		VertexShader(InputLayoutDescription* desc);

		ID3D11InputLayout* getInputLayout();

		void createShader(ID3D11Device* device);
	};
}