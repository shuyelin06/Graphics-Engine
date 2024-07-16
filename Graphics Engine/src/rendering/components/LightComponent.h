#pragma once

#include "rendering/components/ViewComponent.h"

#include "rendering/Shader.h"

#include "rendering/Direct3D11.h"
#include "math/Matrix4.h"

namespace Engine
{
namespace Graphics
{
	// Forward declaration of VisualSystem
	class VisualSystem;

	// LightComponent Class:
	// Represents a directional light. Lights create shadows, and we create them
	// using a shadow mapping technique.
	// The "direction" of the light's view is given by the direction of its rotated +Z
	// axis. To rotate a light, simply rotate its transform.
	class LightComponent
		: public ViewComponent
	{
	private:
		// Shadow map texture, and associated data to
		// let us bind it to the shader and render to it.
		ID3D11Texture2D* shadow_map;
		D3D11_VIEWPORT viewport;

		// Enables rendering to the texture
		ID3D11DepthStencilView* depth_stencil_view;
		
		// Enables use of / sampling of the texture in shaders
		ID3D11ShaderResourceView* shader_resource_view;
		ID3D11SamplerState* sampler_state;

	public:
		LightComponent(Datamodel::Object* object, VisualSystem* system);
		~LightComponent();

		// Set render target to be the texture
		void setRenderTarget(VisualSystem* system);

		// Bind the shadow map to a texture slot
		void bindShadowMap(VisualSystem* system, int slot_index, CBHandle* cbHandle);
	};
}
}