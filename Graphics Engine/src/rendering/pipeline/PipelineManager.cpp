#include "PipelineManager.h"

#include <assert.h>

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
}

PipelineManager::~PipelineManager() {
    for (int i = 0; i < CBSlot::CBCOUNT; i++) {
        delete vcb_handles[i];
        delete pcb_handles[i];
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
    // Save the active vertex shader
    vs_active = shader_manager->getVertexShader(vs_name);

    // Check if shader exists
    if (vs_active == nullptr)
        assert(false);

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

void PipelineManager::drawPostProcessQuad() {
    UINT vertexStride = sizeof(float) * 4;
    UINT vertexOffset = 0;

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

} // namespace Graphics
} // namespace Engine