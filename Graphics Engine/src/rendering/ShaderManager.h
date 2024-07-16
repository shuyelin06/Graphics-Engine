#pragma once

#include "Direct3D11.h"

#include "Shader.h"

namespace Engine
{
namespace Graphics
{
	// Input layout configurations for the vertex shader
	enum InputLayoutPin
	{
		XYZ = 1,			// Position
		RGB = 1 << 1,		// Color
		TEX = 1 << 2,		// Texture Coordinate
		NORMAL = 1 << 3		// Normal Vector
	};

	// Index references to available vertex and pixel 
	// shaders. We use indices so that these
	// shaders can be accessed efficiently
	enum VSSlot
	{
		VSDefault = 0,
		VSShadow = 1,
		VSCount
	};

	enum PSSlot
	{
		PSDefault = 0,
		PSShadow = 1,
		PSCount
	};
	
	// ShaderManager Class:
	// Manages shaders for the engine. Provides methods to create shaders,
	// bind shaders, and validate shader inputs.
	class ShaderManager
	{
	private:
		std::vector<VertexShader*> vertexShaders;
		std::vector<PixelShader*> pixelShaders;

	public:
		ShaderManager();
		~ShaderManager();

		// Load and configure all of the shaders usable by the engine
		void initialize(ID3D11Device* device);

		// Access a shader by its respective enumerator slot.
		VertexShader* getVertexShader(VSSlot slot);
		PixelShader* getPixelShader(PSSlot slot);

	private:
		// Helper functions for compiling and building vertex and pixel shaders
		VertexShader* createVertexShader(ID3D11Device* device, int layout, const wchar_t* filename, const char* entrypoint);
		PixelShader* createPixelShader(ID3D11Device* device, const wchar_t* filename, const char* entrypoint);
	};
}
}