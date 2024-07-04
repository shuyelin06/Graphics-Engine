#include "LightComponent.h"

#include <assert.h>

#include "rendering/VisualSystem.h"

#define SHADOWMAP_WIDTH 256
#define SHADOWMAP_HEIGHT 256

//DXGI_FORMAT_R8G8B8A8_UNORM
namespace Engine
{
namespace Graphics
{
	// LightData Struct:
	// Contains data that the Pixel Shader needs to perform shadow calculations
	struct ShadowData
	{
		Vector3 light_position;
		float padding;
		Matrix4 view_matrix;
		Matrix4 projection_matrix;
	};

	// Constructor:
	// Initializes a texture resource for use in the shadow mapping.
	LightComponent::LightComponent(Datamodel::Object* object, VisualSystem* system)
		: ViewComponent(object, system)
	{
		// Cast the handler to a visual system (TODO: probably a better way to do this)
		VisualSystem* visual_system = system;

		// Get device to create resources. This can probably be moved to the visual system later
		ID3D11Device* device = visual_system->getDevice();

		// Create my texture
		shadow_map = NULL;
		D3D11_TEXTURE2D_DESC texture_description = { };

		texture_description.Width = SHADOWMAP_WIDTH;
		texture_description.Height = SHADOWMAP_HEIGHT;
		texture_description.MipLevels = 1; // No mipmap needed
		texture_description.ArraySize = 1;
		texture_description.Format = DXGI_FORMAT_R24G8_TYPELESS; // Typeless to allow multiple bind flags
		texture_description.SampleDesc.Count = 1;
		texture_description.Usage = D3D11_USAGE_DEFAULT;
		texture_description.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; //  fails...
		texture_description.CPUAccessFlags = 0;
		texture_description.MiscFlags = 0;

		device->CreateTexture2D(&texture_description, NULL, &shadow_map);
		assert(shadow_map != NULL);

		// Initialize a depth stencil view so that the texture
		// can be used as the depth buffer to initialize the shadow map. 
		depth_stencil_view = NULL;
		D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = { };

		// 24 Bits for Depth, 8 Bits for Stencil
		depth_stencil_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_stencil_view_desc.Texture2D.MipSlice = 0;

		device->CreateDepthStencilView(shadow_map, &depth_stencil_view_desc, &depth_stencil_view);
		assert(depth_stencil_view != NULL);

		// Create viewport
		viewport = { };

		viewport.Width = SHADOWMAP_WIDTH;
		viewport.Height = SHADOWMAP_HEIGHT;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;

		// Create a shader resource view so that th texture
		// can be sampled in the shader
		shader_resource_view = NULL;
		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_description = { };

		// 24 Bits Floating Point 0.0 -> 1.0f, 8 Bits Typeless (discarded?)
		shader_resource_view_description.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		shader_resource_view_description.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_description.Texture2D.MostDetailedMip = 0;
		shader_resource_view_description.Texture2D.MipLevels = 1;

		device->CreateShaderResourceView(shadow_map, &shader_resource_view_description, &shader_resource_view);
		assert(shader_resource_view != NULL);
		
		// Create a sampler state
		sampler_state = NULL;
		D3D11_SAMPLER_DESC sampler_desc = {};
		
		sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
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

		device->CreateSamplerState(&sampler_desc, &sampler_state);
	}

	LightComponent::~LightComponent()
	{
		system->removeLightComponent(this);
	}

	// Sets the shadow map as the render target
	void LightComponent::setRenderTarget(VisualSystem* system)
	{
		ID3D11DeviceContext* device_context = system->getDeviceContext();

		// Set render target view and viewport
		device_context->OMSetRenderTargets(0, nullptr, depth_stencil_view);
		device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 0);

		device_context->RSSetViewports(1, &viewport);
	}

	// Binds the shadow map to a texture slot
	void LightComponent::bindShadowMap(VisualSystem* system, int slot_index)
	{
		ID3D11DeviceContext* device_context = system->getDeviceContext();

		// Bind view of the texture
		device_context->PSSetShaderResources(slot_index, 1, &shader_resource_view);
		// Configure sampling of the texture
		device_context->PSSetSamplers(slot_index, 1, &sampler_state);
		
		// Bind light data to CB1
		// Generate view structure data
		ShadowData shadow_data = { };

		shadow_data.light_position = object->getTransform().getPosition();
		shadow_data.padding = 0;
		shadow_data.view_matrix = object->getLocalMatrix().inverse();
		shadow_data.projection_matrix = generateProjectionMatrix();

		// Load this view data into the vertex shader's constant buffer 1
		system->BindPSData(CB_Type::PER_VIEW, &shadow_data, sizeof(ShadowData));
	}

}
}