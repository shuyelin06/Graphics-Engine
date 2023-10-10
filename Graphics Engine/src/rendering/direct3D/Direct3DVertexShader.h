#pragma once

#include "Direct3D11.h"

#include "rendering/core/Renderer.h"
#include "rendering/core/VertexShader.h"

namespace Engine
{

namespace Graphics
{
	// Direct3DVertexShader
	// Vertex shader to be used in the Direct3D
	// graphics api
	class Direct3DVertexShader : public VertexShader
	{
	private:
		ID3D11VertexShader* vertex_shader;
		ID3D11InputLayout* input_layout;

	public:
		Direct3DVertexShader(ID3D11Device* device, const wchar_t* shader_file, const char* entrypoint, std::vector<InputLayout>& layout);

		ID3D11VertexShader* getShader();
		ID3D11InputLayout* getInputLayout();
	};
}
}