#include "ShaderManager.h"

#include <assert.h>

namespace Engine
{
namespace Graphics
{
    ShaderManager::ShaderManager() = default;
	ShaderManager::~ShaderManager() = default;

    // Initialize:
    // Creates and configures all of the shaders usable by the engine.
	void ShaderManager::initialize(ID3D11Device* device)
	{
        vertexShaders.resize(VSSlot::VSCount);
        pixelShaders.resize(PSSlot::PSCount);
        
        const std::wstring shaderFolder = L"src/rendering/shaders/";
        
        vertexShaders[VSDebugPoint] = createVertexShader(device, XYZ | INSTANCE_ID, shaderFolder + L"DebugPointRenderer.hlsl", "vs_main");
        {
            std::vector<CBDataFormat> cb0 = std::vector<CBDataFormat>();
            for (int i = 0; i < 2048; i++)
            {
                cb0.push_back(FLOAT3); // Position
                cb0.push_back(FLOAT);  // Scale
                cb0.push_back(FLOAT3); // Color
            }
            vertexShaders[VSDebugPoint]->enableCB(CB0, cb0.data(), cb0.size());
            CBDataFormat cb1[] = { FLOAT4X4, FLOAT4X4 };
            vertexShaders[VSDebugPoint]->enableCB(CB1, cb1, 2);
        }
        pixelShaders[PSDebugPoint] = createPixelShader(device, shaderFolder + L"DebugPointRenderer.hlsl", "ps_main");

        vertexShaders[VSDebugLine] = createVertexShader(device, XYZ | RGB, shaderFolder + L"DebugLineRenderer.hlsl", "vs_main");
        {
            CBDataFormat cb1[] = { FLOAT4X4, FLOAT4X4 };
            vertexShaders[VSDebugLine]->enableCB(CB1, cb1, 2);
        }
        pixelShaders[PSDebugLine] = createPixelShader(device, shaderFolder + L"DebugLineRenderer.hlsl", "ps_main");

        vertexShaders[VSDefault] = createVertexShader(device, XYZ | TEX | NORMAL, shaderFolder + L"VertexShader.hlsl", "vs_main");
        {
            CBDataFormat cb1[] = { FLOAT4X4, FLOAT4X4 };
            vertexShaders[VSDefault]->enableCB(CB1, cb1, 2);
            CBDataFormat cb2[] = { FLOAT4X4, FLOAT4X4 };
            vertexShaders[VSDefault]->enableCB(CB2, cb2, 2);
        }
        
        pixelShaders[PSDefault] = createPixelShader(device, shaderFolder + L"PixelShader.hlsl", "ps_main");

        vertexShaders[VSShadow] = createVertexShader(device, XYZ | TEX | NORMAL, shaderFolder + L"ShadowShaderV.hlsl", "vs_main");
        {
            CBDataFormat cb1[] = { FLOAT4X4, FLOAT4X4 };
            vertexShaders[VSShadow]->enableCB(CB1, cb1, 2);
            CBDataFormat cb2[] = { FLOAT4X4, FLOAT4X4 };
            vertexShaders[VSShadow]->enableCB(CB2, cb2, 2);
        }

        pixelShaders[PSShadow] = createPixelShader(device, shaderFolder + L"ShadowShaderP.hlsl", "ps_main");
        {
            CBDataFormat cb1[] = { FLOAT3, FLOAT4X4, FLOAT4X4 };
            pixelShaders[PSShadow]->enableCB(CB1, cb1, 3);
        }
	}

    // GetVertexShader:
    // Returns a vertex shader by a given slot, which internally
    // indexes an array.
    VertexShader* ShaderManager::getVertexShader(VSSlot slot)
	{
		assert(0 <= slot && slot < vertexShaders.size());
		return vertexShaders[slot];
	}

    // GetPixelShader:
    // Returns a pixel shader by a given slot, which internally 
    // indexes an array.
	PixelShader* ShaderManager::getPixelShader(PSSlot slot)
	{
		assert(0 <= slot && slot < pixelShaders.size());
		return pixelShaders[slot];
	}

    // CompileShaderBlob:
    // Compiles a file into a shader blob. Used in the creation of vertex
    // and pixel shaders.
    enum ShaderType { Vertex, Pixel };
    static ID3DBlob* CompileShaderBlob(ShaderType type, const std::wstring file, const char* entry)
    {
        // Initialize compiler settings
        ID3DInclude* include_settings = D3D_COMPILE_STANDARD_FILE_INCLUDE;
        const char* compiler_target = "";
        const UINT flags = 0 | D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS;

        switch (type) {
        case Vertex:
            compiler_target = "vs_5_0";
            break;

        case Pixel:
            compiler_target = "ps_5_0";
            break;
        }

        // Compile blob
        ID3DBlob* error_blob = NULL;
        ID3DBlob* compiled_blob = NULL;

        HRESULT result = D3DCompileFromFile(
            file.c_str(),
            nullptr,
            include_settings,
            entry,
            compiler_target,
            flags,
            0,
            &compiled_blob,
            &error_blob
        );

        // Error handling
        if (FAILED(result))
        {
            // Print error if message exists
            if (error_blob)
            {
                OutputDebugStringA((char*)error_blob->GetBufferPointer());
                error_blob->Release();
            }
            // Release shader blob if allocated
            if (compiled_blob) { compiled_blob->Release(); }
            assert(false);
        }

        return compiled_blob;
    }

	VertexShader* ShaderManager::createVertexShader(ID3D11Device* device, int layout, const std::wstring filename, const char* entrypoint)
	{
        // Obtain shader blob
        ID3DBlob* shader_blob = CompileShaderBlob(Vertex, filename, entrypoint);

        // Create input layout for vertex shader
        ID3D11InputLayout* inputLayout = NULL;

        /*
        D3D11_INPUT_ELEMENT_DESC inputDesc[10];
        
        int inputDescSize = 0;
        int floatOffset = 0;

        if (XYZ)
        {
            inputDesc[inputDescSize] =
            {

            };
            inputDescSize += 1;
            floatOffset += 3
        }
        */

        switch (layout)
        {
        case (XYZ | RGB):
        {
            D3D11_INPUT_ELEMENT_DESC input_desc[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };
            int input_desc_size = 2;

            device->CreateInputLayout(
                input_desc,
                input_desc_size,
                shader_blob->GetBufferPointer(),
                shader_blob->GetBufferSize(),
                &inputLayout
            );
        }
        break;

        case (XYZ | INSTANCE_ID):
        {
            D3D11_INPUT_ELEMENT_DESC input_desc[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };
            int input_desc_size = 2;

            device->CreateInputLayout(
                input_desc,
                input_desc_size,
                shader_blob->GetBufferPointer(),
                shader_blob->GetBufferSize(),
                &inputLayout
            );
        }
        break;

        case (XYZ | TEX | NORMAL):
        {
            D3D11_INPUT_ELEMENT_DESC input_desc[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 5, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };
            int input_desc_size = 3;

            device->CreateInputLayout(
                input_desc,
                input_desc_size,
                shader_blob->GetBufferPointer(),
                shader_blob->GetBufferSize(),
                &inputLayout
            );
        }
        break;
        }
        
        assert(inputLayout != NULL);

        // Create vertex shader
        ID3D11VertexShader* vertexShader = NULL;

        device->CreateVertexShader(
            shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(),
            NULL,
            &vertexShader
        );

        // Free shader blob memory
        shader_blob->Release();

        return new VertexShader(vertexShader, inputLayout);
	}

    // CreatePixelShader:
    // Creates a pixel shader and adds it to the array of pixel shaders
	PixelShader* ShaderManager::createPixelShader(ID3D11Device* device, const std::wstring filename, const char* entrypoint)
	{
        // Obtain shader blob
        ID3DBlob* shader_blob = CompileShaderBlob(Pixel, filename, entrypoint);

        // Create pixel shader
        ID3D11PixelShader* pixelShader = NULL;

        device->CreatePixelShader(
            shader_blob->GetBufferPointer(),
            shader_blob->GetBufferSize(),
            NULL,
            &pixelShader
        );

        // Check for success
        assert(pixelShader != NULL);

        // Free shader blob memory
        shader_blob->Release();

        return new PixelShader(pixelShader);
	}

}
}