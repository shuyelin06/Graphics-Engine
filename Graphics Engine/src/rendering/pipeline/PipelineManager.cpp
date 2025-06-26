#include "PipelineManager.h"

#include <assert.h>

#include "../core/Asset.h"
#include "math/Vector4.h"

namespace Engine {
using namespace Math;

namespace Graphics {
PipelineManager::PipelineManager(ID3D11Device* _device,
                                 ID3D11DeviceContext* _context)
    : device(_device), context(_context) {
    // Initialize my shader manager
    shader_manager = new ShaderManager(device);
    shader_manager->initializeShaders();

    // Initialize my vertex buffers / offsets / strides
    active_pool_addr = NULL;
    memset(vb_buffers, 0, sizeof(ID3D11Buffer*) * BINDABLE_STREAM_COUNT);
    memset(vb_strides, 0, sizeof(UINT) * BINDABLE_STREAM_COUNT);
    memset(vb_offsets, 0, sizeof(UINT) * BINDABLE_STREAM_COUNT);

    vb_strides[POSITION] = sizeof(float) * 3;
    vb_strides[TEXTURE] = sizeof(float) * 2;
    vb_strides[NORMAL] = sizeof(float) * 3;
    vb_strides[COLOR] = sizeof(float) * 3;
    vb_strides[DEBUG_LINE] = sizeof(float) * 6;
    vb_strides[SV_POSITION] = sizeof(float) * 4;
    vb_strides[JOINTS] = sizeof(float) * 4;
    vb_strides[WEIGHTS] = sizeof(float) * 4;

    // Initialize my constant buffer handles
    for (int i = 0; i < CBSlot::CBCOUNT; i++) {
        vcb_handles[i] = new CBHandle();
        pcb_handles[i] = new CBHandle();
    }

    // Initialize my full screen quad
    {
        const Vector4 fullscreen_quad[6] = {
            // First Triangle
            Vector4(-1, -1, 0, 1), Vector4(-1, 1, 0, 1), Vector4(1, 1, 0, 1),
            // Second Triangle
            Vector4(-1, -1, 0, 1), Vector4(1, 1, 0, 1), Vector4(1, -1, 0, 1)};

        D3D11_BUFFER_DESC buffer_desc = {};
        buffer_desc.ByteWidth = sizeof(fullscreen_quad);
        buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA sr_data = {};
        sr_data.pSysMem = (void*)fullscreen_quad;

        device->CreateBuffer(&buffer_desc, &sr_data, &postprocess_quad);
    }

    initializeSamplers();
    bindSamplers();
}

PipelineManager::~PipelineManager() {
    for (int i = 0; i < CBSlot::CBCOUNT; i++) {
        delete vcb_handles[i];
        delete pcb_handles[i];
    }

    for (int i = 0; i < SamplerSlot::SamplerCount; i++) {
        if (samplers[i] != nullptr)
            samplers[i]->Release();
    }

    delete shader_manager;
}

// Constant Buffer Loading
IConstantBuffer PipelineManager::loadVertexCB(CBSlot slot) {
    return IConstantBuffer(vcb_handles[slot], slot, CBVertex, device, context);
}
IConstantBuffer PipelineManager::loadPixelCB(CBSlot slot) {
    return IConstantBuffer(pcb_handles[slot], slot, CBPixel, device, context);
}

// --- Pipeline Binding ---
bool PipelineManager::bindVertexShader(const std::string& vs_name) {
    VertexShader* new_shader = shader_manager->getVertexShader(vs_name);

    // Check if shader exists
    if (new_shader == nullptr)
        assert(false);

    // If the new vertex shader has a different layout, invalidate
    // the active mesh pool as we'll have to upload additional stream data
    if (active_pool_addr != NULL && vs_active->layout != new_shader->layout)
        active_pool_addr = NULL;

    // Save the active vertex shader
    vs_active = new_shader;

    // Bind shader and input layout
    context->IASetInputLayout(vs_active->layout);
    context->VSSetShader(vs_active->shader, NULL, 0);

    return true;
}

bool PipelineManager::bindPixelShader(const std::string& ps_name) {
    ps_active = shader_manager->getPixelShader(ps_name);

    // Check if shader exists
    if (ps_active == nullptr)
        assert(false);

    // Bind shader
    context->PSSetShader(ps_active->shader, NULL, 0);

    return true;
}

void PipelineManager::bindSamplers() {
    context->PSSetSamplers(0, SamplerCount, samplers);
}

void PipelineManager::drawMesh(const Mesh* mesh, int tri_start, int tri_end,
                               UINT instance_count) {
    assert((vs_active->layout_pin & mesh->layout) == vs_active->layout_pin);
    const MeshPool* pool = mesh->buffer_pool;

    // All meshes are assumed to havae a triangle list topology.
    // While there are more efficient representations, this is done
    // for simplicity.
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // UNTESTED: If the MeshPool is the same, we don't need to rebind anything
    if (pool != active_pool_addr) {
        active_pool_addr = pool;

        // Bind my index buffer. All meshes are assumed to have one index
        // buffer, associated with multiple vertex buffers.
        context->IASetIndexBuffer(pool->ibuffer, DXGI_FORMAT_R32_UINT, 0);

        // Iterate through the layout of my vertex shader
        // and bind the vertex buffers that the vertex shader needs.
        memset(vb_buffers, 0, sizeof(ID3D11Buffer*) * BINDABLE_STREAM_COUNT);
        for (int i = 0; i < BINDABLE_STREAM_COUNT; i++) {
            if (((1 << i) & vs_active->layout_pin) == (1 << i)) {
                vb_buffers[i] = pool->vbuffers[i];
            }
        }

        context->IASetVertexBuffers(0, BINDABLE_STREAM_COUNT, vb_buffers,
                                    vb_strides, vb_offsets);
    }

    // Issue my draw call. We will always draw indexed instanced, even if the
    // number of instances is 1.
    const UINT index_start = (mesh->triangle_start + tri_start) * 3;
    const UINT num_indices =
        (tri_end == -1) ? mesh->num_triangles * 3 : (tri_end - tri_start) * 3;
    const UINT index_offset = mesh->vertex_start;

    context->DrawIndexedInstanced(num_indices, instance_count, index_start,
                                  index_offset, 0);
}

void PipelineManager::drawPostProcessQuad() {
    const UINT vertexStride = sizeof(float) * 4;
    const UINT vertexOffset = 0;

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(0, 1, &postprocess_quad, &vertexStride,
                                &vertexOffset);

    context->Draw(6, 0);
}

void PipelineManager::initializeTargets(HWND window) {
    // Get my window width and height
    RECT rect;
    GetClientRect(window, &rect);
    const UINT width = rect.right - rect.left;
    const UINT height = rect.bottom - rect.top;

    // Create my swap chain and screen target
}

// InitializeSamplers:
// Initializes the most commonly used samplers in the pipeline.
// These samplers will not be rebound over the entire program.
void PipelineManager::initializeSamplers() {
    D3D11_SAMPLER_DESC sampler_desc = {};
    ID3D11SamplerState* sampler = NULL;

    for (int i = 0; i < SamplerCount; i++)
        samplers[i] = NULL;

    // Point Sampler: Index 0
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    device->CreateSamplerState(&sampler_desc, &sampler);
    assert(sampler != NULL);

    samplers[Point] = sampler;

    // Shadow Sampler: Index 1
    sampler_desc.Filter =
        D3D11_FILTER_MIN_MAG_MIP_LINEAR; // Linear Filtering for PCF
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.BorderColor[0] = 0.f;
    sampler_desc.BorderColor[1] = 0.f;
    sampler_desc.BorderColor[2] = 0.f;
    sampler_desc.BorderColor[3] = 0.f;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0;
    sampler_desc.MaxLOD = 1.0f;

    device->CreateSamplerState(&sampler_desc, &sampler);
    assert(sampler != NULL);

    samplers[Shadow] = sampler;
}

} // namespace Graphics
} // namespace Engine