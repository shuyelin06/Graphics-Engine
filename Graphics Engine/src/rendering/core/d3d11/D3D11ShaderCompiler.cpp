#include "D3D11ShaderCompiler.h"

#include <filesystem>
#include <stdio.h>
#include <string.h>

#include <assert.h>
#include <optional>

constexpr bool ALLOW_CACHING = false;

namespace Engine
{
namespace Graphics
{

static const std::string cache_folder = "bin_shaders/";
static const std::string shader_folder = "shaders/";

D3D11ShaderCompiler::D3D11ShaderCompiler(ID3D11Device* _device)
{
    device = _device;
}
D3D11ShaderCompiler::~D3D11ShaderCompiler() = default;

// InitializeShaders:
// Creates all of the shaders usable by the engine. To create a shader, populate
// the ShaderConfig struct with data, and pass in an array of "pins" (defines).
// We can use pins to make one shader file usable for multiple different input
// types or configurations. Pass in NULL if there are no pins.
// For vertex shaders, an additional array of input layouts is needed.
struct ShaderConfig
{
    ShaderType shader_type;

    std::string shader_name; // Name of Shader in Engine

    std::string source_file; // Source File
    std::string entry_point; // Entrypoint Name

    // Input data semantics; only valid for vertex shaders
    std::vector<VertexDataStream> input_layout;
    // Pins
    std::vector<std::string> pins;
};

void D3D11ShaderCompiler::initializeShaders()
{
    vertex_shaders.clear();
    pixel_shaders.clear();

    const std::vector<ShaderConfig> shaders = {
        // DebugPoint:
        // Uses instancing to draw colored points in the scene. Only
        // available if the debug flag is flipped.
        {Vertex,
         "DebugPoint",
         "misc/DebugPointRenderer.hlsl",
         "vs_main",
         {PosXYZ_TexU, INSTANCE_ID},
         {}},
        {Pixel,
         "DebugPoint",
         "misc/DebugPointRenderer.hlsl",
         "ps_main",
         {},
         {}},
        // DebugLine:
        // Uses instancing to draw colored lines in the scene. Only
        // available if the debug flag is flipped.
        {Vertex,
         "DebugLine",
         "misc/DebugLineRenderer.hlsl",
         "vs_main",
         {PosXYZ_TexU, ColorRGBA},
         {}},
        {Pixel, "DebugLine", "misc/DebugLineRenderer.hlsl", "ps_main"},
        // ShadowMap Shader:
        // Takes vertex triangle data, as well as
        // matrix transforms and writes them to a light's shadow map (depth
        // buffer).
        {Vertex, "ShadowMap", "ShadowMap.hlsl", "vs_main", {PosXYZ_TexU}, {}},
        {Pixel, "ShadowMap", "ShadowMap.hlsl", "ps_main", {}, {}},
        // Terrain Shader:
        // Handles rendering of the scene's terrain.
        {Vertex,
         "Terrain",
         "V_Terrain.hlsl",
         "vsterrain_main",
         {PosXYZ_TexU, INSTANCE_ID},
         {}},
        {Pixel, "Terrain", "P_Terrain.hlsl", "psterrain_main", {}, {}},
        // Shadow:
        // Draws a mesh with dynamic lights enabled
        {Vertex,
         "ShadowShader",
         "ShadowShaderV.hlsl",
         "vs_main",
         {PosXYZ_TexU, NormXYZ_TexV, ColorRGBA},
         {}},
        {Pixel, "ShadowShader", "ShadowShaderP.hlsl", "ps_main", {}, {}},
        // Shadow (Textured):
        // Draws a mesh with dynamic lights enabled
        {Vertex,
         "TexturedMesh",
         "V_TexturedMesh.hlsl",
         "vs_main",
         {PosXYZ_TexU, NormXYZ_TexV, INSTANCE_ID},
         {}},
        {Vertex,
         "SkinnedMesh",
         "V_TexturedMesh.hlsl",
         "vs_main",
         {PosXYZ_TexU, NormXYZ_TexV, JOINTS, WEIGHTS, INSTANCE_ID},
         {"SKINNED_MESH"}},
        {Pixel, "TexturedMesh", "P_TexturedMesh.hlsl", "ps_main", {}, {}},
        // --- Post Processing Effects ---
        // Generic vertex shader for post process effects
        {Vertex,
         "PostProcess",
         "Post_VertexShader.hlsl",
         "vs_main",
         {PosXYZ_TexU},
         {}},
        {Pixel, "PostProcess", "Post_PixelShader.hlsl", "ps_main", {}, {}},
        {Pixel, "Sky", "Post_Abovewater.hlsl", "ps_main", {}, {}},
        {Pixel, "Underwater", "Post_Underwater.hlsl", "ps_main", {}, {}}
        // ...
    };

    for (const ShaderConfig& config : shaders)
    {
        if (config.shader_type == Vertex)
            createVertexShader(config);
        else
            createPixelShader(config);
    }
}

// ShaderIncludeHandler Class:
// Allows shaders to use the #include directive, by searching for the contents
// of the files as given by the directive.
// This class extends the ID3D11Include interface to do this.
class ShaderIncludeHandler : public ID3DInclude
{
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
        if (include_type == D3D_INCLUDE_LOCAL)
        {
            // Open file
            const std::string path =
                shader_folder + "include/" + std::string(file_name);
            FILE* file = fopen(path.c_str(), "rb");
            assert(file != NULL);

            // Read file size
            UINT file_size = (UINT)std::filesystem::file_size(path);
            uint8_t* read_data = new uint8_t[file_size];
            fread(read_data, 1, file_size, file);

            *size = file_size;
            *data = read_data;

            fclose(file);

            return S_OK;
        }
        else
            return E_NOTIMPL;
    }

    HRESULT Close(LPCVOID data) noexcept override
    {
        delete[] data;
        return S_OK;
    }
};

// GetVertexShader:
// Returns a vertex shader by a given slot, which internally
// indexes an array.
D3D11VertexShader* D3D11ShaderCompiler::getVertexShader(const char* name)
{
    auto iter = vertex_shaders.find(name);
    if (iter != vertex_shaders.end())
        return iter->second.get();
    else
        return nullptr;
}

// GetPixelShader:
// Returns a pixel shader by a given slot, which internally
// indexes an array.
D3D11PixelShader* D3D11ShaderCompiler::getPixelShader(const char* name)
{
    auto iter = pixel_shaders.find(name);
    if (iter != pixel_shaders.end())
        return iter->second.get();
    else
        return nullptr;
}

// CompileShaderBlob:
// Compiles a file into a shader blob. Used in the creation of vertex
// and pixel shaders.
ID3DBlob* D3D11ShaderCompiler::compileShaderBlob(ShaderType type,
                                                 const ShaderConfig& config)
{
    ID3DBlob* compiled_blob = NULL;

    // Generate path to shader file
    const std::string shader_path = shader_folder + config.source_file;
    const std::wstring shader_path_w =
        std::wstring(shader_path.begin(), shader_path.end());

    std::string cached_blob_path =
        cache_folder + config.source_file + "--" + config.entry_point;
    if (!config.pins.empty())
    {
        for (const std::string& pin : config.pins)
        {
            cached_blob_path += ", " + pin;
        }
    }
    const std::wstring cached_blob_path_w =
        std::wstring(cached_blob_path.begin(), cached_blob_path.end());

    if (std::filesystem::exists(cached_blob_path) && ALLOW_CACHING)
    {
        // If the blob was last modified after the shader, then it is the most
        // up-to-date blob for the shader and we don't need to recompile.
        auto blob_last_modified =
            std::filesystem::last_write_time(cached_blob_path);
        auto shader_last_modified =
            std::filesystem::last_write_time(shader_path);

        if (blob_last_modified >= shader_last_modified)
        {
            D3DReadFileToBlob(cached_blob_path_w.c_str(), &compiled_blob);

            if (compiled_blob != NULL)
                return compiled_blob;
        }
    }

    // Check if the blob has already been compiled before.
    // Initialize compiler settings
    ID3DInclude* include_settings = new ShaderIncludeHandler();
    const char* compiler_target = "";
    const UINT flags = 0 | D3DCOMPILE_DEBUG | D3DCOMPILE_ENABLE_STRICTNESS;

    switch (type)
    {
    case Vertex:
        compiler_target = "vs_5_0";
        break;

    case Pixel:
        compiler_target = "ps_5_0";
        break;
    }

    // Parse the pins. These are defines that can enable / disable portions of
    // the shader code.
    D3D_SHADER_MACRO* macros = NULL;

    if (!config.pins.empty())
    {
        shader_macros.clear();

        const std::vector<std::string>& pins = config.pins;
        for (const std::string& pin : pins)
        {
            D3D_SHADER_MACRO macro;
            macro.Name = pin.c_str();
            macro.Definition = NULL;
            shader_macros.push_back(macro);
        }

        shader_macros.push_back({NULL, NULL});
        macros = shader_macros.data();
    }

    ID3DBlob* error_blob = NULL;

    // Compile blob
    HRESULT result =
        D3DCompileFromFile(shader_path_w.c_str(), macros, include_settings,
                           config.entry_point.c_str(), compiler_target, flags,
                           0, &compiled_blob, &error_blob);

    delete include_settings;

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
        if (compiled_blob)
        {
            compiled_blob->Release();
        }
        assert(false);
    }

    // Cache blob so that we don't have to recompile in the future
    D3DWriteBlobToFile(compiled_blob, cached_blob_path_w.c_str(), true);

    return compiled_blob;
}

void D3D11ShaderCompiler::createVertexShader(const ShaderConfig& config)
{
    // Obtain shader blob
    ID3DBlob* shader_blob = compileShaderBlob(Vertex, config);

    // Create input layout for vertex shader. We do this by parsing the streams
    // that the shader will use into the corresponding input data format.
    ID3D11InputLayout* inputLayout = NULL;

    std::vector<D3D11_INPUT_ELEMENT_DESC> input_desc;

    for (const VertexDataStream& stream : config.input_layout)
    {
        D3D11_INPUT_ELEMENT_DESC desc;

        switch (stream)
        {
        case PosXYZ_TexU:
            desc = {"PosXYZ_TexU",
                    0,
                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                    PosXYZ_TexU,
                    0,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0};
            break;
        case NormXYZ_TexV:
            desc = {"NormXYZ_TexV",
                    0,
                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                    NormXYZ_TexV,
                    0,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0};
            break;
        case ColorRGBA:
            desc = {"ColorRGBA",
                    0,
                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                    ColorRGBA,
                    0,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0};
            break;

        // Joints ID Stream:
        // A buffer of integers, which index a joint array for the asset.
        // This array tells us what joints influence a mesh in an asset.
        case JOINTS:
            desc = {"JOINTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,
                    JOINTS,   0, D3D11_INPUT_PER_VERTEX_DATA,
                    0};
            break;
        // Weights ID Stream:
        // A buffer of floats, telling us how much a joint influences a vertex.
        case WEIGHTS:
            desc = {"WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,
                    WEIGHTS,   0, D3D11_INPUT_PER_VERTEX_DATA,
                    0};
            break;

        // Instance ID Stream:
        // A buffer of instance IDs, which can be used in instance rendering
        case INSTANCE_ID:
            desc = {"SV_InstanceID",
                    0,
                    DXGI_FORMAT_R32_UINT,
                    INSTANCE_ID,
                    0,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0};
            break;

        // Vertex ID Stream:
        // A buffer of vertex IDs, which can be used in vertex pulling
        case VERTEX_ID:
            desc = {"SV_VertexID",
                    0,
                    DXGI_FORMAT_R32_UINT,
                    VERTEX_ID,
                    0,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0};
            break;
        }

        input_desc.push_back(desc);
    }

    device->CreateInputLayout(input_desc.data(), (UINT)input_desc.size(),
                              shader_blob->GetBufferPointer(),
                              shader_blob->GetBufferSize(), &inputLayout);
    assert(inputLayout != NULL);

    // Create vertex shader
    ID3D11VertexShader* vertexShader = NULL;

    device->CreateVertexShader(shader_blob->GetBufferPointer(),
                               shader_blob->GetBufferSize(), NULL,
                               &vertexShader);
    shader_blob->Release(); // Free shader blob memory

    // Create my vertex shader
    vertex_shaders[config.shader_name] =
        std::make_unique<D3D11VertexShader>(vertexShader, inputLayout);
}

// CreatePixelShader:
// Creates a pixel shader and adds it to the array of pixel shaders
void D3D11ShaderCompiler::createPixelShader(const ShaderConfig& config)
{
    assert(config.input_layout.empty());

    // Obtain shader blob
    ID3DBlob* shader_blob = compileShaderBlob(Pixel, config);

    // Create pixel shader
    ID3D11PixelShader* pixelShader = NULL;

    device->CreatePixelShader(shader_blob->GetBufferPointer(),
                              shader_blob->GetBufferSize(), NULL, &pixelShader);

    // Check for success
    assert(pixelShader != NULL);

    // Free shader blob memory
    shader_blob->Release();

    pixel_shaders[config.shader_name] =
        std::make_unique<D3D11PixelShader>(pixelShader);
}

} // namespace Graphics
} // namespace Engine