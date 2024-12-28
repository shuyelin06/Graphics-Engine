#include "ShaderManager.h"

#include <filesystem>
#include <stdio.h>
#include <string.h>

#include <assert.h>

namespace Engine {
namespace Graphics {

static const std::string shader_folder = "src/rendering/shaders/";

// ShaderIncludeHandler Class:
// Allows shaders to use the #include directive, by searching for the contents
// of the files as given by the directive.
// This class extends the ID3D11Include interface to do this.
class ShaderIncludeHandler : public ID3DInclude {
  public:
    ShaderIncludeHandler() = default;
    ~ShaderIncludeHandler() = default;

    HRESULT Open(D3D_INCLUDE_TYPE include_type, // User or System Include
                 LPCSTR file_name,              // Included File Name
                 LPCVOID parent_data,           // Data Passed to Compiler
                 LPCVOID* data, // Output Data (Return to compiler)
                 UINT* size) noexcept override // Size of data
    {
        // This includer handler only supports user includes. Others (like
        // system includes) should be handled by the system itself.
        if (include_type == D3D_INCLUDE_LOCAL) {
            // Open file
            const std::string path =
                shader_folder + "include/" + std::string(file_name);
            FILE* file = fopen(path.c_str(), "rb");
            assert(file != NULL);

            // Read file size
            uint32_t file_size = std::filesystem::file_size(path);
            uint8_t* read_data = new uint8_t[file_size];
            fread(read_data, 1, file_size, file);

            *size = file_size;
            *data = read_data;

            fclose(file);

            return S_OK;
        } else
            return E_NOTIMPL;
    }

    HRESULT Close(LPCVOID data) noexcept override {
        delete[] data;
        return S_OK;
    }
};

ShaderManager::ShaderManager(ID3D11Device* _device) { device = _device; }
ShaderManager::~ShaderManager() = default;

// Initialize:
// Creates and configures all of the shaders usable by the engine.
void ShaderManager::initialize() {
    vertexShaders.resize(VSSlot::VSCount);
    pixelShaders.resize(PSSlot::PSCount);

    // ShadowMap Shader:
    // A very simple shader that takes vertex triangle data, as well as matrix transforms and writes them to a
    // light's shadow map (depth buffer).
    vertexShaders[VSShadowMap] =
        createVertexShader("ShadowMap.hlsl", "vs_main", XYZ);
    vertexShaders[VSShadowMap]->enableCB(CB0);
    vertexShaders[VSShadowMap]->enableCB(CB1);
    pixelShaders[PSShadowMap] = createPixelShader("ShadowMap.hlsl", "ps_main");

    vertexShaders[VSDebugPoint] = createVertexShader(
        "DebugPointRenderer.hlsl", "vs_main", XYZ | INSTANCE_ID);
    vertexShaders[VSDebugPoint]->enableCB(CB0);
    vertexShaders[VSDebugPoint]->enableCB(CB1);
    pixelShaders[PSDebugPoint] =
        createPixelShader("DebugPointRenderer.hlsl", "ps_main");

    vertexShaders[VSDebugLine] =
        createVertexShader("DebugLineRenderer.hlsl", "vs_main", XYZ | RGB);
    vertexShaders[VSDebugLine]->enableCB(CB1);
    pixelShaders[PSDebugLine] =
        createPixelShader("DebugLineRenderer.hlsl", "ps_main");

    vertexShaders[VSDefault] =
        createVertexShader("VertexShader.hlsl", "vs_main", XYZ | TEX | NORMAL);
    vertexShaders[VSDefault]->enableCB(CB1);
    vertexShaders[VSDefault]->enableCB(CB2);
    pixelShaders[PSDefault] = createPixelShader("PixelShader.hlsl", "ps_main");

    vertexShaders[VSShadow] =
        createVertexShader("ShadowShaderV.hlsl", "vs_main", XYZ | TEX | NORMAL);
    vertexShaders[VSShadow]->enableCB(CB1);
    vertexShaders[VSShadow]->enableCB(CB2);
    pixelShaders[PSShadow] = createPixelShader("ShadowShaderP.hlsl", "ps_main");
    pixelShaders[PSShadow]->enableCB(CB0); // Global Illumination
    pixelShaders[PSShadow]->enableCB(CB1);
}

// GetVertexShader:
// Returns a vertex shader by a given slot, which internally
// indexes an array.
VertexShader* ShaderManager::getVertexShader(VSSlot slot) {
    assert(0 <= slot && slot < vertexShaders.size());
    return vertexShaders[slot];
}

// GetPixelShader:
// Returns a pixel shader by a given slot, which internally
// indexes an array.
PixelShader* ShaderManager::getPixelShader(PSSlot slot) {
    assert(0 <= slot && slot < pixelShaders.size());
    return pixelShaders[slot];
}

// CompileShaderBlob:
// Compiles a file into a shader blob. Used in the creation of vertex
// and pixel shaders.
enum ShaderType { Vertex, Pixel };
static ID3DBlob* CompileShaderBlob(ShaderType type, const std::string file,
                                   const char* entry) {
    // Initialize compiler settings
    const std::wstring file_name = std::wstring(file.begin(), file.end());
    ID3DInclude* include_settings = new ShaderIncludeHandler();
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
        file_name.c_str(), nullptr, include_settings, entry, compiler_target,
        flags, 0, &compiled_blob, &error_blob);

    // Error handling
    if (FAILED(result)) {
        // Print error if message exists
        if (error_blob) {
            OutputDebugStringA((char*)error_blob->GetBufferPointer());
            error_blob->Release();
        }
        // Release shader blob if allocated
        if (compiled_blob) {
            compiled_blob->Release();
        }
        assert(false);
    }

    return compiled_blob;
}

VertexShader* ShaderManager::createVertexShader(const std::string filename,
                                                const char* entrypoint,
                                                int layout) {
    // Obtain shader blob
    ID3DBlob* shader_blob =
        CompileShaderBlob(Vertex, shader_folder + filename, entrypoint);

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

    switch (layout) {
    case (XYZ): {
        D3D11_INPUT_ELEMENT_DESC input_desc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
             D3D11_INPUT_PER_VERTEX_DATA, 0}};
        int input_desc_size = 1;

        device->CreateInputLayout(input_desc, input_desc_size,
                                  shader_blob->GetBufferPointer(),
                                  shader_blob->GetBufferSize(), &inputLayout);
    } break;

    case (XYZ | RGB): {
        D3D11_INPUT_ELEMENT_DESC input_desc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 3,
             D3D11_INPUT_PER_VERTEX_DATA, 0}};
        int input_desc_size = 2;

        device->CreateInputLayout(input_desc, input_desc_size,
                                  shader_blob->GetBufferPointer(),
                                  shader_blob->GetBufferSize(), &inputLayout);
    } break;

    case (XYZ | INSTANCE_ID): {
        D3D11_INPUT_ELEMENT_DESC input_desc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 3,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 5,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"SV_InstanceID", 0, DXGI_FORMAT_R32_UINT, 0, sizeof(float) * 8,
             D3D11_INPUT_PER_VERTEX_DATA, 0}};
        int input_desc_size = 4;

        device->CreateInputLayout(input_desc, input_desc_size,
                                  shader_blob->GetBufferPointer(),
                                  shader_blob->GetBufferSize(), &inputLayout);
    } break;

    case (XYZ | TEX | NORMAL): {
        D3D11_INPUT_ELEMENT_DESC input_desc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 3,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 5,
             D3D11_INPUT_PER_VERTEX_DATA, 0}};
        int input_desc_size = 3;

        device->CreateInputLayout(input_desc, input_desc_size,
                                  shader_blob->GetBufferPointer(),
                                  shader_blob->GetBufferSize(), &inputLayout);
    } break;
    }

    assert(inputLayout != NULL);

    // Create vertex shader
    ID3D11VertexShader* vertexShader = NULL;

    device->CreateVertexShader(shader_blob->GetBufferPointer(),
                               shader_blob->GetBufferSize(), NULL,
                               &vertexShader);

    // Free shader blob memory
    shader_blob->Release();

    return new VertexShader(vertexShader, inputLayout);
}

// CreatePixelShader:
// Creates a pixel shader and adds it to the array of pixel shaders
PixelShader* ShaderManager::createPixelShader(const std::string filename,
                                              const char* entrypoint) {
    // Obtain shader blob
    ID3DBlob* shader_blob =
        CompileShaderBlob(Pixel, shader_folder + filename, entrypoint);

    // Create pixel shader
    ID3D11PixelShader* pixelShader = NULL;

    device->CreatePixelShader(shader_blob->GetBufferPointer(),
                              shader_blob->GetBufferSize(), NULL, &pixelShader);

    // Check for success
    assert(pixelShader != NULL);

    // Free shader blob memory
    shader_blob->Release();

    return new PixelShader(pixelShader);
}

} // namespace Graphics
} // namespace Engine