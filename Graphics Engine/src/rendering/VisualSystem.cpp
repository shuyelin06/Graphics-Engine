#include "VisualSystem.h"

#include <assert.h>
#include <d3d11_1.h>

#include "VisualDebug.h"

#include "datamodel/Object.h"

#define RGB(rgb) ((rgb) / 255.f)

// TODO: Debug drawing w/ instancing 
// device_context->DrawIndexedInstanced(num_indices, VisualDebug::points.size(), 0, 0, 1);

namespace Engine
{

namespace Graphics
{
    // Constructor
    // Saves the handle to the application window and initializes the
    // system's data structures
    VisualSystem::VisualSystem(HWND _window)
    {
        window = _window;

        // Components
        lightComponents = std::vector<LightComponent*>();
        assetComponents = std::vector<AssetComponent*>();
        viewComponents = std::vector<ViewComponent*>();

        // Managers
        assetManager = AssetManager();
        shaderManager = ShaderManager();

        device = NULL;
        context = NULL;
        swap_chain = NULL;
        render_target_view = NULL;
        depth_stencil = NULL;
    }

    // Initialize: 
    // Initializes Direct3D11 for use in rendering
    void VisualSystem::initialize()
    {
        // Get window width and height
        RECT rect;
        GetWindowRect(window, &rect);

        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        /* Initialize Swap Chain */
        // Populate a Swap Chain Structure, responsible for swapping between textures (for rendering)
        DXGI_SWAP_CHAIN_DESC swap_chain_descriptor = { 0 };
        swap_chain_descriptor.BufferDesc.RefreshRate.Numerator = 0; // Synchronize Output Frame Rate
        swap_chain_descriptor.BufferDesc.RefreshRate.Denominator = 1;
        swap_chain_descriptor.BufferDesc.Width = width;
        swap_chain_descriptor.BufferDesc.Height = height;
        swap_chain_descriptor.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // Color Output Format
        swap_chain_descriptor.SampleDesc.Count = 1;
        swap_chain_descriptor.SampleDesc.Quality = 0;
        swap_chain_descriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_descriptor.BufferCount = 1; // Number of back buffers to add to the swap chain
        swap_chain_descriptor.OutputWindow = window;
        swap_chain_descriptor.Windowed = true; // Displaying to a Window

        // Create swap chain, and save pointer data
        D3D_FEATURE_LEVEL feature_level;

        HRESULT result = D3D11CreateDeviceAndSwapChain(
            NULL,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            D3D11_CREATE_DEVICE_SINGLETHREADED, // Flags
            NULL,
            0,
            D3D11_SDK_VERSION,
            &swap_chain_descriptor,
            &swap_chain,
            &device,
            &feature_level,
            &context);

        // Check for success
        assert(S_OK == result && swap_chain && device && context);


        /* Create Render Target (Output Images) */
        // Obtain Frame Buffer
        ID3D11Texture2D* framebuffer;

        // Create Render Target with Frame Buffer
        {
            result = swap_chain->GetBuffer(
                0, // Get Buffer 0
                __uuidof(ID3D11Texture2D),
                (void**)&framebuffer);

            // Check for success
            assert(SUCCEEDED(result));

            // Create Render Target with Frame Buffer
            result = device->CreateRenderTargetView(
                framebuffer, 0, &render_target_view);
            // Check Success
            assert(SUCCEEDED(result));
        }

        // Release frame buffer (no longer needed)
        framebuffer->Release();

        // Create 2D texture to be used as a depth stencil
        ID3D11Texture2D* depth_texture = CreateTexture2D(D3D11_BIND_DEPTH_STENCIL, width, height);

        // Create a depth stencil from the 2D texture
        {
            // Create description for the depth stencil
            D3D11_DEPTH_STENCIL_VIEW_DESC desc_stencil = {};
            desc_stencil.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Same format as texture
            desc_stencil.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

            // Create depth stencil
            device->CreateDepthStencilView(depth_texture, &desc_stencil, &depth_stencil);
        }

        assetManager.initialize();
        shaderManager.initialize(device);
    }

    // Render:
    // Renders the entire scene to the screen.
    void VisualSystem::render()
    {
        // Clear the the screen color
        float color[4] = { RGB(158.f), RGB(218.f), RGB(255.f), 1.0f };
        context->ClearRenderTargetView(render_target_view, color);

        performShadowPass();
        performRenderPass();
        
#if defined(_DEBUG)
        // Debug Functionality
        renderDebugPoints();
        // renderDebugLines();
        VisualDebug::Clear();
#endif

        // Present what we rendered to
        swap_chain->Present(1, 0);
    }

    void VisualSystem::performShadowPass()
    {
        VertexShader* vShader = shaderManager.getVertexShader(VSDefault);
        PixelShader* pShader = shaderManager.getPixelShader(PSDefault);

        // Render the scene to each light's shadow map.
        for (LightComponent* light : lightComponents)
        {
            vShader->getCBHandle(CB1)->clearData();
            light->loadViewData(vShader->getCBHandle(CB1));

            light->setRenderTarget(context);

            for (AssetComponent* asset: assetComponents)
            {
                asset->beginLoading();

                vShader->getCBHandle(CB2)->clearData();
                int numIndices = asset->loadMeshData(context, vShader->getCBHandle(CB2), device);

                while (numIndices != -1)
                {
                    vShader->bindShader(device, context);
                    pShader->bindShader(device, context);
                    context->DrawIndexed(numIndices, 0, 0);

                    vShader->getCBHandle(CB2)->clearData();
                    numIndices = asset->loadMeshData(context, vShader->getCBHandle(CB2), device);
                }
            }
        }
    }

    void VisualSystem::performRenderPass()
    {
        VertexShader* vShader = shaderManager.getVertexShader(VSShadow);
        PixelShader* pShader = shaderManager.getPixelShader(PSShadow);

        ViewComponent* view = viewComponents[0];

        context->OMSetRenderTargets(1, &render_target_view, depth_stencil);
        context->ClearDepthStencilView(depth_stencil, D3D11_CLEAR_DEPTH, 1.0f, 0);
        D3D11_VIEWPORT viewport = getViewport();
        context->RSSetViewports(1, &viewport);

        vShader->getCBHandle(CB1)->clearData();
        view->loadViewData(vShader->getCBHandle(CB1));

        CBHandle* pCB1 = pShader->getCBHandle(CB1);
        pCB1->clearData();

        const Vector3& viewPosition = view->getObject()->getTransform().getPosition();
        pCB1->loadData(&viewPosition, FLOAT3);
        
        int lightCount = lightComponents.size();
        pCB1->loadData(&lightCount, INT);

        for (int i = 0; i < lightComponents.size(); i++)
        {
            lightComponents[i]->loadLightData(pCB1);
            lightComponents[i]->bindShadowMap(context, i, pCB1);
        }

        for (AssetComponent* asset_component : assetComponents)
        {
            asset_component->beginLoading();

            vShader->getCBHandle(CB2)->clearData();
            int numIndices = asset_component->loadMeshData(context, vShader->getCBHandle(CB2), device);

            while (numIndices != -1)
            {
                vShader->bindShader(device, context);
                pShader->bindShader(device, context);
                context->DrawIndexed(numIndices, 0, 0);

                vShader->getCBHandle(CB2)->clearData();
                numIndices = asset_component->loadMeshData(context, vShader->getCBHandle(CB2), device);
            }
        }
    }

    void VisualSystem::renderDebugPoints()
    {
        VertexShader* vShader = shaderManager.getVertexShader(VSDebugPoint);
        PixelShader* pShader = shaderManager.getPixelShader(PSDebugPoint);
        ViewComponent* view = viewComponents[0];

        vShader->getCBHandle(CB0)->clearData();
        vShader->getCBHandle(CB1)->clearData();

        Asset* cube = assetManager.getAsset(Cube);
        int numIndices = cube->getMesh(0)->loadIndexVertexData(context, device);

        int numPoints = VisualDebug::LoadPointData(vShader->getCBHandle(CB0));

        if (numPoints > 0)
        {
            view->loadViewData(vShader->getCBHandle(CB1));

            vShader->bindShader(device, context);
            pShader->bindShader(device, context);

            context->DrawIndexedInstanced(numIndices, numPoints, 0, 0, 1);
        }
    }

    void VisualSystem::renderDebugLines()
    {
        VertexShader* vShader = shaderManager.getVertexShader(VSDebugLine);
        PixelShader* pShader = shaderManager.getPixelShader(PSDebugLine);
        ViewComponent* view = viewComponents[0];

        vShader->getCBHandle(CB1)->clearData();

        int numLines = VisualDebug::LoadLineData(context, device);
        
        if (numLines > 0)
        {
            view->loadViewData(vShader->getCBHandle(CB1));

            vShader->bindShader(device, context);
            pShader->bindShader(device, context);

            context->Draw(numLines, 0);
        }
    }

    // GetViewport:
    // Returns the current viewport
    D3D11_VIEWPORT VisualSystem::getViewport() const
    {
        // Get the window rectangle
        RECT winRect;
        GetClientRect(window, &winRect);

        // Generate viewport description
        D3D11_VIEWPORT viewport = {
            0.0f, 0.0f,
            (float) (winRect.right - winRect.left),
            (float) (winRect.bottom - winRect.top),
            0.0f,
            1.0f
        };

        // Set depth for the depth buffer
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;

        // Return
        return viewport;
    }

    // GetActiveCamera:
    // Returns the currently active camera in the scene.
    ViewComponent* VisualSystem::getActiveView()
    {
        // If there are no camera components, return nullptr.
        if (viewComponents.size() == 0)
            return nullptr;
        
        // Otherwise, (for now) just return the first one.
        return viewComponents[0];
    }
    
    // CreateTexture2D:
    // Creates a 2D texture for use in the rendering pipeline.
    ID3D11Texture2D* VisualSystem::CreateTexture2D(D3D11_BIND_FLAG bind_flag, int width, int height)
    {
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

    // --- Component Handling ----
    // MeshComponent:
    // Allows the rendering of a mesh using the object's transform.
    // Meshes have material properties, which specify how they are to be rendered.
    AssetComponent* VisualSystem::bindAssetComponent(Datamodel::Object* object, AssetSlot assetName)
    {
        // Create component
        Asset* asset = assetManager.getAsset(assetName);
        AssetComponent* component = new AssetComponent(object, this, asset);
        assetComponents.push_back(component);

        // Register with object
        object->registerComponent<AssetComponent>(component);

        return component;
    }

    bool VisualSystem::removeAssetComponent(AssetComponent* component)
    {
        auto index = std::find(assetComponents.begin(), assetComponents.end(), component);

        if (index != assetComponents.end())
        {
            assetComponents.erase(index);
            return true;
        }
        else
            return false;
    }

    // ViewComponent:
    // Contains data needed to render a scene from a particular view.
    // This view can be used as a camera, or used to render to a texture
    // (for techniques like shadowmapping)
    ViewComponent* VisualSystem::bindViewComponent(Datamodel::Object* object)
    {
        // Create component
        ViewComponent* component = new ViewComponent(object, this);
        viewComponents.push_back(component);

        // Register with object
        object->registerComponent<ViewComponent>(component);

        return component;
    }

    bool VisualSystem::removeViewComponent(ViewComponent* component)
    {
        auto index = std::find(viewComponents.begin(), viewComponents.end(), component);

        if (index != viewComponents.end())
        {
            viewComponents.erase(index);
            return true;
        }
        else
            return false;
    }

    // LightComponent:
    // Used to convert an object to a light source with shadows.
    LightComponent* VisualSystem::bindLightComponent(Datamodel::Object* object)
    {
        // Create component
        LightComponent* component = new LightComponent(object, this, device);
        lightComponents.push_back(component);

        // Register with object
        object->registerComponent<LightComponent>(component);

        return component;
    }
    
    bool VisualSystem::removeLightComponent(LightComponent* component)
    {
        auto index = std::find(lightComponents.begin(), lightComponents.end(), component);

        if (index != lightComponents.end())
        {
            lightComponents.erase(index);
            return true;
        }
        else
            return false;
    }

}
}