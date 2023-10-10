#pragma once

#include <vector>

#include "PixelShader.h"
#include "VertexShader.h"

#include "Buffer.h"

using namespace std;

namespace Engine
{

namespace Graphics 
{

	// Specifies the supported input layout types,
	// which will be passed into the vertex shader 
	typedef enum {Position3} InputLayout;
	
	// Specifies the layout of the vertices 
	// being passed into the vertex buffer
	typedef enum {TriangleList} InputTopology;

	// Specifies where we can bind our buffers
	typedef enum {Vertex, Pixel} BufferTarget;

	// Renderer Class
	// Provides the high-level interface which can
	// be used to interact with a given lower-level
	// graphics API
	class Renderer
	{
	public:
		virtual void Render() = 0; 

		virtual void BindVertexBuffer(InputTopology, Buffer*, unsigned int vertex_size) = 0;
		virtual void BindConstantBuffer(BufferTarget, Buffer*, int index) = 0;

		virtual void BindVertexShader(VertexShader*) = 0;
		virtual void BindPixelShader(PixelShader*) = 0;

		virtual Buffer* CreateBuffer(void* data, int size) = 0;
		// virtual // Buffer* CreateVertexBuffer(void* data, int size);

		virtual PixelShader* CreatePixelShader(const wchar_t* shader_file, const char* entrypoint) = 0;
		virtual VertexShader* CreateVertexShader(const wchar_t* shader_file, const char* entrypoint, vector<InputLayout>& layout) = 0;
	};
}
}