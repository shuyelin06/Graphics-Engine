#include "LightComponent.h"

#include <assert.h>

#include "rendering/VisualSystem.h"

#define SHADOWMAP_WIDTH 1024
#define SHADOWMAP_HEIGHT 1024

//DXGI_FORMAT_R8G8B8A8_UNORM
namespace Engine
{
namespace Graphics
{
	// Constructor:
	// Initializes a texture resource for use in the shadow mapping.
	LightComponent::LightComponent(Datamodel::ComponentHandler<LightComponent>* handler)
		: Datamodel::Component<LightComponent>(handler)
	{
		// Cast the handler to a visual system (TODO: probably a better way to do this)
		visual_system = static_cast<VisualSystem*>(handler);

		fov = 1.2f;
		z_near = 1.f;
		z_far = 200.f;

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

		depth_stencil_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Format for depth buffer
		depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depth_stencil_view_desc.Texture2D.MipSlice = 0;

		device->CreateDepthStencilView(shadow_map, &depth_stencil_view_desc, &depth_stencil_view);
		assert(depth_stencil_view != NULL);

		// Create a shader resource view so that th texture
		// can be sampled in the shader
		shader_resource_view = NULL;
		D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_description = { };

		shader_resource_view_description.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		shader_resource_view_description.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shader_resource_view_description.Texture2D.MostDetailedMip = 0;
		shader_resource_view_description.Texture2D.MipLevels = 1;

		device->CreateShaderResourceView(shadow_map, &shader_resource_view_description, &shader_resource_view);
		assert(shader_resource_view != NULL);
		
		// Create viewport
		viewport = { };

		viewport.Width = SHADOWMAP_WIDTH;
		viewport.Height = SHADOWMAP_HEIGHT;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
	}

	// Sets the shadow map as the render target
	void LightComponent::setRenderTarget()
	{
		ID3D11DeviceContext* device_context = visual_system->getDeviceContext();

		// Set render target view and viewport
		device_context->OMSetRenderTargets(0, nullptr, depth_stencil_view);
		device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 0);

		device_context->RSSetViewports(1, &viewport);
	}

	ID3D11ShaderResourceView* LightComponent::getShaderResourceView()
	{
		return shader_resource_view;
	}

	// GetProjectionMatrix:
	// Generates and returns the projection matrix for the light.
	Matrix4 LightComponent::getProjectionMatrix(void) const
	{
		float fov_factor = cosf(fov / 2.f) / sinf(fov / 2.f);

		Matrix4 projection_matrix = Matrix4();

		projection_matrix[0][0] = fov_factor;
		projection_matrix[1][1] = fov_factor;
		projection_matrix[2][2] = z_far / (z_far - z_near);
		projection_matrix[2][3] = 1;
		projection_matrix[3][2] = (z_near * z_far) / (z_near - z_far);
		
		return projection_matrix;
	}
}
}