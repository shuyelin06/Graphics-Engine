#include "ShaderManager.h"

#include <filesystem>
#include <stdio.h>
#include <string.h>

#include <assert.h>

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

// Initialize:
// Creates and configures all of the shaders usable by the engine.
void ShaderManager::initialize() {
    // ShadowMap Shader:
    // A very simple shader that takes vertex triangle data, as well as matrix
    // transforms and writes them to a light's shadow map (depth buffer).
    {
        VertexDataStream shadow_map_input[] = {POSITION};
        VertexShader* vs = createVertexShader("ShadowMap.hlsl", "vs_main",
                                              shadow_map_input, 1);
        vs->enableCB(CB0);
        vs->enableCB(CB1);
        vertex_shaders["ShadowMap"] = vs;

        PixelShader* ps = createPixelShader("ShadowMap.hlsl", "ps_main");
        pixel_shaders["ShadowMap"] = ps;
    }

    // Terrain Shader:
    // Handles rendering of the scene's terrain. Done in a separate shader than
    // the meshes as terrain is procedurally textured with a tri-planar mapping
    {
        VertexDataStream terrain_input[] = {POSITION, NORMAL};
        VertexShader* vs = createVertexShader(
            "VSTerrain.hlsl", "vsterrain_main", terrain_input, 2);
        vs->enableCB(CB0);
        vs->enableCB(CB1);
        vertex_shaders["Terrain"] = vs;

        PixelShader* ps = createPixelShader("PSTerrain.hlsl", "psterrain_main");
        ps->enableCB(CB0); // Global Illumination
        ps->enableCB(CB1);
        pixel_shaders["Terrain"] = ps;
    }

    // DebugPoint:
    // Uses instancing to draw colored points in the scene. Only available if
    // the debug flag is flipped.
    {
        VertexDataStream debug_point_input[2] = {POSITION, INSTANCE_ID};
        VertexShader* vs = createVertexShader("DebugPointRenderer.hlsl",
                                              "vs_main", debug_point_input, 2);
        vs->enableCB(CB0);
        vs->enableCB(CB1);
        vertex_shaders["DebugPoint"] = vs;

        PixelShader* ps =
            createPixelShader("DebugPointRenderer.hlsl", "ps_main");
        pixel_shaders["DebugPoint"] = ps;
    }

    // DebugLine:
    // Uses instancing to draw colored lines in the scene. Only available if the
    // debug flag is flipped.
    {
        VertexDataStream debug_line_input[1] = {DEBUG_LINE};
        VertexShader* vs = createVertexShader("DebugLineRenderer.hlsl",
                                              "vs_main", debug_line_input, 1);
        vs->enableCB(CB1);
        vertex_shaders["DebugLine"] = vs;

        PixelShader* ps =
            createPixelShader("DebugLineRenderer.hlsl", "ps_main");
        pixel_shaders["DebugLine"] = ps;
    }

    /*vertexShaders[VSDefault] =
        createVertexShader("VertexShader.hlsl", "vs_main", XYZ | TEX | NORMAL);
    vertexShaders[VSDefault]->enableCB(CB1);
    vertexShaders[VSDefault]->enableCB(CB2);
    pixelShaders[PSDefault] = createPixelShader("PixelShader.hlsl",
    "ps_main");*/

    // Shadow:
    // Draws a mesh with dynamic lights enabled
    {
        VertexDataStream shadow_input[3] = {POSITION, NORMAL, COLOR};
        VertexShader* vs = createVertexShader("ShadowShaderV.hlsl", "vs_main",
                                              shadow_input, 3);
        vs->enableCB(CB1);
        vs->enableCB(CB2);
        vertex_shaders["ShadowShader"] = vs;

        PixelShader* ps = createPixelShader("ShadowShaderP.hlsl", "ps_main");
        ps->enableCB(CB0); // Global Illumination
        ps->enableCB(CB1);
        pixel_shaders["ShadowShader"] = ps;
    }

    // Blur:
    // Attempt at blurring
    {
        VertexDataStream blur_input[1] = {SV_POSITION};
        VertexShader* vs =
            createVertexShader("Blur.hlsl", "vs_blur", blur_input, 1);
        vertex_shaders["Blur"] = vs;

        PixelShader* ps = createPixelShader("Blur.hlsl", "ps_blur");
        ps->enableCB(CB0);
        pixel_shaders["Blur"] = ps;
    }
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
enum ShaderType { Vertex, Pixel };
static ID3DBlob* CompileShaderBlob(ShaderType type, const std::string& file,
                                   const char* entry) {
    ID3DBlob* compiled_blob = NULL;

    // Generate path to shader file
    const std::string shader_path = shader_folder + file;
    const std::wstring shader_path_w =
        std::wstring(shader_path.begin(), shader_path.end());

    const std::string cached_blob_path = cache_folder + file + "--" + entry;
    const std::wstring cached_blob_path_w =
        std::wstring(cached_blob_path.begin(), cached_blob_path.end());

    if (std::filesystem::exists(cached_blob_path)) {
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

    // Compile blob
    ID3DBlob* error_blob = NULL;

    HRESULT result = D3DCompileFromFile(
        shader_path_w.c_str(), nullptr, include_settings, entry,
        compiler_target, flags, 0, &compiled_blob, &error_blob);

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

VertexShader* ShaderManager::createVertexShader(const std::string& filename,
                                                const char* entrypoint,
                                                VertexDataStream* input_data,
                                                UINT input_data_size) {
    // Obtain shader blob
    ID3DBlob* shader_blob = CompileShaderBlob(Vertex, filename, entrypoint);

    // Create input layout for vertex shader. We do this by parsing the streams
    // that the shader will use into the corresponding input data format.
    ID3D11InputLayout* inputLayout = NULL;

    std::vector<D3D11_INPUT_ELEMENT_DESC> input_desc;

    for (UINT i = 0; i < input_data_size; i++) {
        const VertexDataStream stream = input_data[i];
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

    // Free shader blob memory
    shader_blob->Release();

    return new VertexShader(vertexShader, inputLayout);
}

// CreatePixelShader:
// Creates a pixel shader and adds it to the array of pixel shaders
PixelShader* ShaderManager::createPixelShader(const std::string& filename,
                                              const char* entrypoint) {
    // Obtain shader blob
    ID3DBlob* shader_blob = CompileShaderBlob(Pixel, filename, entrypoint);

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