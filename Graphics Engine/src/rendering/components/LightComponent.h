#pragma once

#include "datamodel/ComponentHandler.h"
#include "datamodel/Component.h"

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
		: public Datamodel::Component<LightComponent>
	{
	private:
		VisualSystem* visual_system;

		// Light "view" properties
		float z_near;
		float z_far;
		float fov;

		// Shadow map texture, and associated data to
		// let us bind it to the shader and render to it.
		ID3D11Texture2D* shadow_map;
		D3D11_VIEWPORT viewport;

		ID3D11DepthStencilView* depth_stencil_view;
		ID3D11ShaderResourceView* shader_resource_view;
		

	public:
		LightComponent(Datamodel::ComponentHandler<LightComponent>* handler);
	
		// Set render target
		void setRenderTarget();

		ID3D11ShaderResourceView* getShaderResourceView();

		// Generate projection matrix 
		Matrix4 getProjectionMatrix(void) const;
	};
}
}