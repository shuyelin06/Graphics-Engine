#include "PipelineManager.h"

#include <assert.h>

namespace Engine {
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
}

PipelineManager::~PipelineManager() {
    for (int i = 0; i < CBSlot::CBCOUNT; i++) {
        delete vcb_handles[i];
        delete pcb_handles[i];
    }

    delete shader_manager;
}

// --- Accessors ---
CBHandle* PipelineManager::getVertexCB(CBSlot slot) const {
    return vcb_handles[slot];
}

CBHandle* PipelineManager::getPixelCB(CBSlot slot) const {
    return pcb_handles[slot];
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

void PipelineManager::bindVertexCB(CBSlot slot) {
    CBHandle* cb = vcb_handles[slot];

    // Reupload CB data to the GPU
    updateCBData(cb);

    // Bind constant buffer to pipeline
    if (cb->byteSize() > 0)
        context->VSSetConstantBuffers(slot, 1, &cb->resource);
}

void PipelineManager::bindPixelCB(CBSlot slot) {
    CBHandle* cb = pcb_handles[slot];

    // Reupload CB data to the GPU
    updateCBData(cb);

    // Bind constant buffer to pipeline
    if (cb->byteSize() > 0)
        context->PSSetConstantBuffers(slot, 1, &cb->resource);
}

void PipelineManager::updateCBData(CBHandle* constantBuffer) {
    // Do nothing if CB has nothing
    if (constantBuffer->byteSize() == 0)
        return;

    // If the buffer resource has never been created before, or the current
    // resource is too small for mapping / unmapping, create a new one.
    if (constantBuffer->resource == nullptr ||
        constantBuffer->byteSize() > constantBuffer->buffer_size) {
        if (constantBuffer->resource != nullptr)
            constantBuffer->resource->Release();

        // Create buffer to allow dynamic usage, i.e. accessible by
        // GPU read and CPU write. We opt for this usage so that we can update
        // the resource on the fly when needed.
        D3D11_BUFFER_DESC buff_desc = {};
        buff_desc.ByteWidth = constantBuffer->byteSize();
        buff_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        buff_desc.Usage = D3D11_USAGE_DYNAMIC;
        buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        // Allocate resources
        D3D11_SUBRESOURCE_DATA sr_data;
        sr_data.pSysMem = constantBuffer->data.data();
        sr_data.SysMemPitch = 0;
        sr_data.SysMemSlicePitch = 0;

        // Create buffer
        HRESULT result = device->CreateBuffer(&buff_desc, &sr_data,
                                              &(constantBuffer->resource));
        assert(SUCCEEDED(result));

        constantBuffer->buffer_size = constantBuffer->byteSize();
    }
    // If buffer exists, perform resource renaming to update buffer data
    // instead of creating a new buffer
    else {
        // Disable GPU access to data and obtain the my constant buffer resource
        D3D11_MAPPED_SUBRESOURCE mapped_resource = {0};
        context->Map(constantBuffer->resource, 0, D3D11_MAP_WRITE_DISCARD, 0,
                     &mapped_resource);

        // Update the data in the resource
        memcpy(mapped_resource.pData, constantBuffer->data.data(),
               constantBuffer->byteSize());

        // Reenable GPU access to data
        context->Unmap(constantBuffer->resource, 0);
    }
}

} // namespace Graphics
} // namespace Engine