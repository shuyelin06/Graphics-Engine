#include "VisualSystem.h"

#include <assert.h>
#include <d3d11_1.h>

#include "VisualDebug.h"

#include "datamodel/Object.h"

#define RGB(rgb) ((rgb) / 255.f)

// TODO: Debug drawing w/ instancing
// device_context->DrawIndexedInstanced(num_indices, VisualDebug::points.size(),
// 0, 0, 1);

namespace Engine {

namespace Graphics {
AssetRenderRequest::AssetRenderRequest(AssetSlot _slot,
                                       const Matrix4& _mLocalToWorld) {
    slot = _slot;
    mLocalToWorld = _mLocalToWorld;
}

// Constructor
// Saves the handle to the application window and initializes the
// system's data structures
VisualSystem::VisualSystem(HWND _window) {
    window = _window;

    camera = Camera();
    lights = std::vector<Light*>();

    // Managers
    shaderManager = ShaderManager();
    assetManager = nullptr;

    device = NULL;
    context = NULL;
    swap_chain = NULL;
    render_target_view = NULL;
    depth_stencil = NULL;
}

// GetCamera:
// Returns the camera
const Camera& VisualSystem::getCamera() const { return camera; }

Camera& VisualSystem::getCamera() { return camera; }

// Initialize:
// Initializes Direct3D11 for use in rendering
void VisualSystem::initialize() {
    // Get window width and height
    RECT rect;
    GetWindowRect(window, &rect);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    /* Initialize Swap Chain */
    // Populate a Swap Chain Structure, responsible for swapping between
    // textures (for rendering)
    DXGI_SWAP_CHAIN_DESC swap_chain_descriptor = {0};
    swap_chain_descriptor.BufferDesc.RefreshRate.Numerator =
        0; // Synchronize Output Frame Rate
    swap_chain_descriptor.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_descriptor.BufferDesc.Width = width;
    swap_chain_descriptor.BufferDesc.Height = height;
    swap_chain_descriptor.BufferDesc.Format =
        DXGI_FORMAT_B8G8R8A8_UNORM; // Color Output Format
    swap_chain_descriptor.SampleDesc.Count = 1;
    swap_chain_descriptor.SampleDesc.Quality = 0;
    swap_chain_descriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_descriptor.BufferCount =
        1; // Number of back buffers to add to the swap chain
    swap_chain_descriptor.OutputWindow = window;
    swap_chain_descriptor.Windowed = true; // Displaying to a Window

    // Create swap chain, and save pointer data
    D3D_FEATURE_LEVEL feature_level;

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        D3D11_CREATE_DEVICE_SINGLETHREADED, // Flags
        NULL, 0, D3D11_SDK_VERSION, &swap_chain_descriptor, &swap_chain,
        &device, &feature_level, &context);

    // Check for success
    assert(S_OK == result && swap_chain && device && context);

    /* Create Render Target (Output Images) */
    // Obtain Frame Buffer
    ID3D11Texture2D* framebuffer;

    // Create Render Target with Frame Buffer
    {
        result = swap_chain->GetBuffer(0, // Get Buffer 0
                                       __uuidof(ID3D11Texture2D),
                                       (void**)&framebuffer);

        // Check for success
        assert(SUCCEEDED(result));

        // Create Render Target with Frame Buffer
        result =
            device->CreateRenderTargetView(framebuffer, 0, &render_target_view);
        // Check Success
        assert(SUCCEEDED(result));
    }

    // Release frame buffer (no longer needed)
    framebuffer->Release();

    // Create 2D texture to be used as a depth stencil
    ID3D11Texture2D* depth_texture =
        CreateTexture2D(D3D11_BIND_DEPTH_STENCIL, width, height);

    // Create a depth stencil from the 2D texture
    {
        // Create description for the depth stencil
        D3D11_DEPTH_STENCIL_VIEW_DESC desc_stencil = {};
        desc_stencil.Format =
            DXGI_FORMAT_D24_UNORM_S8_UINT; // Same format as texture
        desc_stencil.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

        // Create depth stencil
        device->CreateDepthStencilView(depth_texture, &desc_stencil,
                                       &depth_stencil);
    }

    assetManager = new AssetManager(device);
    assetManager->initialize();
    shaderManager.initialize(device);
}

// CreateLight:
// Creates and returns a light in the visual system
Light* VisualSystem::createLight() {
    Light* newLight = new Light(device);
    lights.push_back(newLight);
    return newLight;
}

// DrawAsset:
// Generates an asset render request
void VisualSystem::drawAsset(AssetSlot asset, const Matrix4& mLocalToWorld) {
    assetRequests.push_back(AssetRenderRequest(asset, mLocalToWorld));
}

// Render:
// Renders the entire scene to the screen.
void VisualSystem::render() {
    // Clear the the screen color
    float color[4] = {RGB(158.f), RGB(218.f), RGB(255.f), 1.0f};
    context->ClearRenderTargetView(render_target_view, color);

    performShadowPass();
    performRenderPass();
    assetRequests.clear();

#if defined(_DEBUG)
    // Debug Functionality
    renderDebugPoints();
    renderDebugLines();
    VisualDebug::Clear();
#endif

    // Present what we rendered to
    swap_chain->Present(1, 0);
}

// PerformShadowPass:
// Render the scene from each light's point of view, to populate
// its shadow map.
void VisualSystem::performShadowPass() {
    VertexShader* vShader = shaderManager.getVertexShader(VSDefault);
    CBHandle* vCB1 = vShader->getCBHandle(CB1);
    CBHandle* vCB2 = vShader->getCBHandle(CB2);

    PixelShader* pShader = shaderManager.getPixelShader(PSDefault);

    for (Light* light : lights) {
        // Populate CB1 of the vertex shader, which will
        // contain the light view and projection matrix.
        // Load light view and projection matrix
        vCB1->clearData();
        {
            const Matrix4 viewMatrix = light->getWorldToCameraMatrix();
            vCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix = light->getProjectionMatrix();
            vCB1->loadData(&projectionMatrix, FLOAT4X4);
        }

        // Set the light as the render target.
        context->ClearDepthStencilView(light->getDepthView(), D3D11_CLEAR_DEPTH,
                                       1.0f, 0);
        context->OMSetRenderTargets(0, nullptr, light->getDepthView());
        context->RSSetViewports(1, &light->getViewport());

        // For each asset, load its mesh
        for (const AssetRenderRequest& assetRequest : assetRequests) {
            const Matrix4& mLocalToWorld = assetRequest.mLocalToWorld;
            Asset* asset = assetManager->getAsset(assetRequest.slot);

            vCB2->clearData();
            {
                // Load mesh vertex transformation matrix
                vCB2->loadData(&mLocalToWorld, FLOAT4X4);
                // Load mesh normal transformation matrix
                Matrix4 normalTransform = mLocalToWorld.inverse().transpose();
                vCB2->loadData(&(normalTransform), FLOAT4X4);
            }

            // Load each mesh
            for (Mesh* mesh : asset->getMeshes()) {
                ID3D11Buffer* indexBuffer = mesh->index_buffer;
                ID3D11Buffer* vertexBuffer = mesh->vertex_buffer;
                int numIndices = mesh->triangle_count * 3;

                UINT vertexStride = sizeof(MeshVertex);
                UINT vertexOffset = 0;

                context->IASetPrimitiveTopology(
                    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                context->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride,
                                            &vertexOffset);
                context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

                vShader->bindShader(device, context);
                pShader->bindShader(device, context);
                context->DrawIndexed(numIndices, 0, 0);
            }
        }
    }
}

void VisualSystem::performRenderPass() {
    VertexShader* vShader = shaderManager.getVertexShader(VSShadow);
    CBHandle* vCB1 = vShader->getCBHandle(CB1);
    CBHandle* vCB2 = vShader->getCBHandle(CB2);

    PixelShader* pShader = shaderManager.getPixelShader(PSShadow);
    CBHandle* pCB1 = pShader->getCBHandle(CB1);

    context->OMSetRenderTargets(1, &render_target_view, depth_stencil);
    context->ClearDepthStencilView(depth_stencil, D3D11_CLEAR_DEPTH, 1.0f, 0);
    D3D11_VIEWPORT viewport = getViewport();
    context->RSSetViewports(1, &viewport);

    // Vertex Constant Buffer 1:
    // Stores the camera view and projection matrices
    vCB1->clearData();
    {
        const Matrix4 viewMatrix = camera.getWorldToCameraMatrix();
        vCB1->loadData(&viewMatrix, FLOAT4X4);

        const Matrix4 projectionMatrix = camera.getProjectionMatrix();
        vCB1->loadData(&projectionMatrix, FLOAT4X4);
    }

    // Pixel Constant Buffer 1:
    // Stores camera position, and the scene's light instances.
    pCB1->clearData();
    {
        const Vector3& cameraPosition = camera.getTransform()->getPosition();
        pCB1->loadData(&cameraPosition, FLOAT3);

        int lightCount = lights.size();
        pCB1->loadData(&lightCount, INT);

        for (int i = 0; i < lights.size(); i++) {
            Light* light = lights[i];

            const Vector3& position = light->getTransform()->getPosition();
            pCB1->loadData(&position, FLOAT3);
            pCB1->loadData(nullptr, FLOAT);

            const Color& color = light->getColor();
            pCB1->loadData(&color, FLOAT3);
            pCB1->loadData(nullptr, FLOAT);

            const Matrix4 viewMatrix = light->getWorldToCameraMatrix();
            pCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix = light->getProjectionMatrix();
            pCB1->loadData(&projectionMatrix, FLOAT4X4);

            // DEBUG
            const Matrix4 frustumMatrix =
                viewMatrix.inverse() * projectionMatrix.inverse();
            VisualDebug::DrawFrustum(frustumMatrix, Color::Green());
        }
    }

    // Load my Textures
    {
        Texture* tex = assetManager->getTexture(Test);
        context->PSSetShaderResources(0, 1, &tex->view);

        // Load light textures and samplers.
        for (int i = 0; i < lights.size(); i++) {
            Light* light = lights[i];
            context->PSSetShaderResources(i + 1, 1, &light->getShaderView());
        }
    }

    // Load my Samplers
    {
        ID3D11SamplerState* mesh_texture_sampler =
            assetManager->getSampler(MeshTexture);
        context->PSSetSamplers(0, 1, &mesh_texture_sampler);

        ID3D11SamplerState* shadowmap_sampler =
            assetManager->getSampler(ShadowMap);
        context->PSSetSamplers(1, 1, &shadowmap_sampler);
    }

    for (const AssetRenderRequest& assetRequest : assetRequests) {
        const Matrix4& mLocalToWorld = assetRequest.mLocalToWorld;
        Asset* asset = assetManager->getAsset(assetRequest.slot);

        {
            vCB2->clearData();
            // Load mesh vertex transformation matrix
            vCB2->loadData(&mLocalToWorld, FLOAT4X4);
            // Load mesh normal transformation matrix
            Matrix4 normalTransform = mLocalToWorld.inverse().transpose();
            vCB2->loadData(&(normalTransform), FLOAT4X4);
        }

        // Load each mesh
        for (Mesh* mesh : asset->getMeshes()) {
            ID3D11Buffer* indexBuffer = mesh->index_buffer;
            ID3D11Buffer* vertexBuffer = mesh->vertex_buffer;
            int numIndices = mesh->triangle_count * 3;

            UINT vertexStride = sizeof(MeshVertex);
            UINT vertexOffset = 0;

            context->IASetPrimitiveTopology(
                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride,
                                        &vertexOffset);
            context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

            vShader->bindShader(device, context);
            pShader->bindShader(device, context);

            context->DrawIndexed(numIndices, 0, 0);
        }
    }
}

void VisualSystem::renderDebugPoints() {
    VertexShader* vShader = shaderManager.getVertexShader(VSDebugPoint);
    CBHandle* vCB1 = vShader->getCBHandle(CB1);

    PixelShader* pShader = shaderManager.getPixelShader(PSDebugPoint);

    vShader->getCBHandle(CB0)->clearData();
    vShader->getCBHandle(CB1)->clearData();

    Asset* cube = assetManager->getAsset(Cube);
    Mesh* mesh = cube->getMesh(0);

    ID3D11Buffer* indexBuffer = mesh->index_buffer;
    ID3D11Buffer* vertexBuffer = mesh->vertex_buffer;
    int numIndices = mesh->triangle_count * 3;

    UINT vertexStride = sizeof(MeshVertex);
    UINT vertexOffset = 0;

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride,
                                &vertexOffset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    vShader->bindShader(device, context);
    pShader->bindShader(device, context);

    int numPoints = VisualDebug::LoadPointData(vShader->getCBHandle(CB0));

    if (numPoints > 0) {
        // Vertex Constant Buffer 1:
        // Stores the camera view and projection matrices
        vCB1->clearData();
        {
            const Matrix4 viewMatrix = camera.getWorldToCameraMatrix();
            vCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix = camera.getProjectionMatrix();
            vCB1->loadData(&projectionMatrix, FLOAT4X4);
        }

        vShader->bindShader(device, context);
        pShader->bindShader(device, context);

        context->DrawIndexedInstanced(numIndices, numPoints, 0, 0, 1);
    }
}

void VisualSystem::renderDebugLines() {
    VertexShader* vShader = shaderManager.getVertexShader(VSDebugLine);
    CBHandle* vCB1 = vShader->getCBHandle(CB1);

    PixelShader* pShader = shaderManager.getPixelShader(PSDebugLine);

    vShader->getCBHandle(CB1)->clearData();

    int numLines = VisualDebug::LoadLineData(context, device);

    if (numLines > 0) {
        // Vertex Constant Buffer 1:
        // Stores the camera view and projection matrices
        vCB1->clearData();
        {
            const Matrix4 viewMatrix = camera.getWorldToCameraMatrix();
            vCB1->loadData(&viewMatrix, FLOAT4X4);

            const Matrix4 projectionMatrix = camera.getProjectionMatrix();
            vCB1->loadData(&projectionMatrix, FLOAT4X4);
        }

        vShader->bindShader(device, context);
        pShader->bindShader(device, context);

        context->Draw(numLines, 0);
    }
}

// GetViewport:
// Returns the current viewport
D3D11_VIEWPORT VisualSystem::getViewport() const {
    // Get the window rectangle
    RECT winRect;
    GetClientRect(window, &winRect);

    // Generate viewport description
    D3D11_VIEWPORT viewport = {0.0f,
                               0.0f,
                               (float)(winRect.right - winRect.left),
                               (float)(winRect.bottom - winRect.top),
                               0.0f,
                               1.0f};

    // Set depth for the depth buffer
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;

    // Return
    return viewport;
}

// CreateTexture2D:
// Creates a 2D texture for use in the rendering pipeline.
ID3D11Texture2D* VisualSystem::CreateTexture2D(D3D11_BIND_FLAG bind_flag,
                                               int width, int height) {
    ID3D11Texture2D* texture = NULL;

    // Create texture description
    D3D11_TEXTURE2D_DESC desc_texture = {};
    desc_texture.Width = width;
    desc_texture.Height = height;
    desc_texture.MipLevels = 1;
    desc_texture.ArraySize = 1;
    desc_texture.MipLevels = 1;
    desc_texture.ArraySize = 1;
    desc_texture.SampleDesc.Count = 1;
    desc_texture.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc_texture.BindFlags = bind_flag;

    // Create 2D texture
    HRESULT result = device->CreateTexture2D(&desc_texture, NULL, &texture);

    // Check for failure
    if (result != S_OK)
        assert(false);

    return texture;
}

} // namespace Graphics
} // namespace Engine