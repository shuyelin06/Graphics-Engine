#include "ShaderManager.h"

#include <filesystem>
#include <stdio.h>
#include <string.h>

#include <assert.h>

constexpr bool ALLOW_CACHING = true;

namespace Engine {
namespace Graphics {

static const std::string cache_folder = "bin/";
static const std::string shader_folder = "shaders/";

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
            UINT file_size = (UINT)std::filesystem::file_size(path);
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

// InitializeShaders:
// Creates all of the shaders usable by the engine. To create a shader, populate
// the ShaderConfig struct with data, and pass in an array of "pins" (defines).
// We can use pins to make one shader file usable for multiple different input
// types or configurations. Pass in NULL if there are no pins.
// For vertex shaders, an additional array of input layouts is needed.
struct ShaderConfig {
    std::string shader_name; // Name of Shader in Engine

    std::string source_file; // Source File
    std::string entry_point; // Entrypoint Name

    bool use_pins;
};

void ShaderManager::initializeShaders() {
    // ShadowMap Shader:
    // A very simple shader that takes vertex triangle data, as well as matrix
    // transforms and writes them to a light's shadow map (depth buffer).
    input_layout_arr = {POSITION};
    createVertexShader({"ShadowMap", "ShadowMap.hlsl", "vs_main"});

    createPixelShader({"ShadowMap", "ShadowMap.hlsl", "ps_main"});

    // Terrain Shader:
    // Handles rendering of the scene's terrain. Done in a separate shader than
    // the meshes as terrain is procedurally textured with a tri-planar mapping
    input_layout_arr = {POSITION, NORMAL};
    createVertexShader({"Terrain", "V_Terrain.hlsl", "vsterrain_main"});

    createPixelShader({"Terrain", "P_Terrain.hlsl", "psterrain_main"});

    // DebugPoint:
    // Uses instancing to draw colored points in the scene. Only available if
    // the debug flag is flipped.
    input_layout_arr = {POSITION, INSTANCE_ID};
    createVertexShader({"DebugPoint", "DebugPointRenderer.hlsl", "vs_main"});

    createPixelShader({"DebugPoint", "DebugPointRenderer.hlsl", "ps_main"});

    // DebugLine:
    // Uses instancing to draw colored lines in the scene. Only available if the
    // debug flag is flipped.
    input_layout_arr = {DEBUG_LINE};
    createVertexShader({"DebugLine", "DebugLineRenderer.hlsl", "vs_main"});

    createPixelShader({"DebugLine", "DebugLineRenderer.hlsl", "ps_main"});

    // Shadow:
    // Draws a mesh with dynamic lights enabled
    input_layout_arr = {POSITION, NORMAL, COLOR};
    createVertexShader({"ShadowShader", "ShadowShaderV.hlsl", "vs_main"});
    createPixelShader({"ShadowShader", "ShadowShaderP.hlsl", "ps_main"});

    // Shadow (Textured):
    // Draws a mesh with dynamic lights enabled
    input_layout_arr = {POSITION, TEXTURE, NORMAL};
    createVertexShader({"TexturedMesh", "V_TexturedMesh.hlsl", "vs_main"});

    input_layout_arr = {POSITION, TEXTURE, NORMAL, JOINTS, WEIGHTS};
    pins_arr = {"SKINNED_MESH"};
    createVertexShader({"SkinnedMesh", "V_TexturedMesh.hlsl", "vs_main", true});

    createPixelShader({"TexturedMesh", "P_TexturedMesh.hlsl", "ps_main"});

    input_layout_arr = {POSITION, INSTANCE_ID};
    createVertexShader(
        {"LightFrustum", "V_LightFrustum.hlsl", "vs_main", false});
    createPixelShader({"LightFrustum", "P_LightFrustum.hlsl", "ps_main"});

    input_layout_arr = {POSITION, INSTANCE_ID};
    createVertexShader(
        {"WaterSurface", "V_WaterSurface.hlsl", "vs_main", false});
    createPixelShader({"WaterSurface", "P_WaterSurface.hlsl", "ps_main"});

    // --- Post Processing Effects ---
    // Generic vertex shader for post process effects
    input_layout_arr = {SV_POSITION};
    createVertexShader({"PostProcess", "Post_VertexShader.hlsl", "vs_main"});
    createPixelShader({"PostProcess", "Post_PixelShader.hlsl", "ps_main"});

    // Sky:
    // Draws a sun and shades the sky
    createPixelShader({"Sky", "Post_Abovewater.hlsl", "ps_main"});

    // Underwater:
    // Creates an underwater effect
    createPixelShader({"Underwater", "Post_Underwater.hlsl", "ps_main"});
}

// GetVertexShader:
// Returns a vertex shader by a given slot, which internally
// indexes an array.
VertexShader* ShaderManager::getVertexShader(const std::string& name) {
    if (vertex_shaders.contains(name))
        return vertex_shaders[name];
    else
        return nullptr;
}

// GetPixelShader:
// Returns a pixel shader by a given slot, which internally
// indexes an array.
PixelShader* ShaderManager::getPixelShader(const std::string& name) {
    if (pixel_shaders.contains(name))
        return pixel_shaders[name];
    else
        return nullptr;
}

// CompileShaderBlob:
// Compiles a file into a shader blob. Used in the creation of vertex
// and pixel shaders.
ID3DBlob* ShaderManager::compileShaderBlob(ShaderType type,
                                           const ShaderConfig& config) {
    ID3DBlob* compiled_blob = NULL;

    // Generate path to shader file
    const std::string shader_path = shader_folder + config.source_file;
    const std::wstring shader_path_w =
        std::wstring(shader_path.begin(), shader_path.end());

    std::string cached_blob_path =
        cache_folder + config.source_file + "--" + config.entry_point;
    if (config.use_pins) {
        for (const std::string& pin : pins_arr) {
            cached_blob_path += ", " + pin;
        }
    }
    const std::wstring cached_blob_path_w =
        std::wstring(cached_blob_path.begin(), cached_blob_path.end());

    if (std::filesystem::exists(cached_blob_path) && ALLOW_CACHING) {
        // If the blob was last modified after the shader, then it is the most
        // up-to-date blob for the shader and we don't need to recompile.
        auto blob_last_modified =
            std::filesystem::last_write_time(cached_blob_path);
        auto shader_last_modified =
            std::filesystem::last_write_time(shader_path);

        if (blob_last_modified >= shader_last_modified) {
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

    switch (type) {
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

    if (config.use_pins) {
        shader_macros.clear();

        const std::vector<std::string>& pins = pins_arr;
        for (const std::string& pin : pins) {
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

    // Cache blob so that we don't have to recompile in the future
    D3DWriteBlobToFile(compiled_blob, cached_blob_path_w.c_str(), true);

    return compiled_blob;
}

void ShaderManager::createVertexShader(const ShaderConfig& config) {
    // Obtain shader blob
    ID3DBlob* shader_blob = compileShaderBlob(Vertex, config);

    // Create input layout for vertex shader. We do this by parsing the streams
    // that the shader will use into the corresponding input data format.
    ID3D11InputLayout* inputLayout = NULL;

    std::vector<D3D11_INPUT_ELEMENT_DESC> input_desc;

    for (const VertexDataStream& stream : input_layout_arr) {
        D3D11_INPUT_ELEMENT_DESC desc;

        switch (stream) {
        // Position Stream:
        // A buffer of (x,y,z) floats for 3D position
        case POSITION:
            desc = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
                    POSITION,   0, D3D11_INPUT_PER_VERTEX_DATA,
                    0};
            break;

        case SV_POSITION: {
            desc = {"SV_POSITION",
                    0,
                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                    0,
                    0,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0};
        } break;

        // Texture Stream:
        // A buffer of (u,v) floats as texture coordinates
        case TEXTURE:
            desc = {"TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT,
                    TEXTURE,   0, D3D11_INPUT_PER_VERTEX_DATA,
                    0};
            break;

        // Normal Stream:
        // A buffer of (x,y,z) normal directions
        case NORMAL:
            desc = {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
                    NORMAL,   0, D3D11_INPUT_PER_VERTEX_DATA,
                    0};
            break;

        // Color Stream:
        // A buffer of RGB colors
        case COLOR:
            desc = {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,
                    COLOR,   0, D3D11_INPUT_PER_VERTEX_DATA,
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

        // Debug Line:
        // A buffer of positions and colors, used for rendering lines
        case DEBUG_LINE: {
            desc = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
                    DEBUG_LINE, 0, D3D11_INPUT_PER_VERTEX_DATA,
                    0};
            input_desc.push_back(desc);
            desc = {"COLOR",
                    0,
                    DXGI_FORMAT_R32G32B32_FLOAT,
                    DEBUG_LINE,
                    D3D11_APPEND_ALIGNED_ELEMENT,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0};
        }
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
    VertexShader* v_shader = new VertexShader(vertexShader, inputLayout);
    for (const VertexDataStream& stream : input_layout_arr)
        v_shader->layout_pin |= (1 << stream);

    vertex_shaders[config.shader_name] = v_shader;
}

// CreatePixelShader:
// Creates a pixel shader and adds it to the array of pixel shaders
void ShaderManager::createPixelShader(const ShaderConfig& config) {
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

    PixelShader* p_shader = new PixelShader(pixelShader);
    pixel_shaders[config.shader_name] = p_shader;
}

} // namespace Graphics
} // namespace Engine