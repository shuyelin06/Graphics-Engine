#pragma once

#include "Direct3D11.h"

#include "datamodel/Object.h"
#include "datamodel/Scene.h"
#include "datamodel/Camera.h"

#include <map>
#include <vector>
#include <utility>

namespace Engine::Datamodel { class Scene; class Object; class Camera; }

using namespace std;

namespace Engine
{
using namespace Datamodel;
namespace Graphics
{
	// Represents shader types in a more readable format (internal use)
	typedef enum { Vertex, Pixel } Shader_Type;

	// Uses bitwise pins to represent the data layout for vertex shaders. 
	// Assumes data is given from least to most significant bit (Right -> Left)
	//     1st Bit) Position (X,Y,Z)
	//     2nd Bit) Color (R,G,B)
	//     3rd Bit) Normal (X,Y,Z)
	// Input data MUST be in this order.
	enum VertexLayoutPin {
		XYZ = 1,
		RGB = (1 << 1),
		NORMAL = (1 << 2)
	};

	// Create and Interpret Vertex Layouts
	int VertexLayoutSize(char layout);
	bool LayoutHasPin(char layout, VertexLayoutPin pin);

	// VisualEngine Class:
	// Provides an interface for the application's graphics
	class VisualAttribute
	{
	/* Visual Attribute Engine 
		Static class that contains all of the Direct3D code necessary
		for rendering in the program. */
	// Static Class Variables
	protected:
		// Handle to Window
		static HWND window;

		// Direct 3D 11 Pointers
		static ID3D11Device* device;
		static ID3D11DeviceContext* device_context;
		static IDXGISwapChain* swap_chain;
		
		// Rendering Views
		static ID3D11RenderTargetView* render_target_view;
		static ID3D11DepthStencilView* depth_stencil;

		// Available Constant Buffers
		static vector<ID3D11Buffer*> vs_buffers;
		static vector<ID3D11Buffer*> ps_buffers;

		// Pipeline Assignable Elements
		static map<char, ID3D11InputLayout*> input_layouts; // Input Layouts
		static vector<ID3D11VertexShader*> vertex_shaders;	// Vertex Shaders
		static vector<ID3D11PixelShader*> pixel_shaders;	// Pixel Shaders
	
		// Visual Attributes
		static vector<VisualAttribute*> attributes;		

		// Camera to render from
		static Camera* camera;

	// Public Static Class Methods
	// Callable by people wanting to render a scene
	public:
		// Initialize Direct 3D and Other Visual Attribute Properties
		static void Initialize(HWND _window);

		// Render All Subscribed Visual Attributes from a Camera
		static void RenderAll();
		
		// TEMP: Set Camera
		static void SetCamera(Camera* _camera);

	// Protected Static Class Methods
	// Callable by the VisualAttributes
	protected:
		// Get an accessor to access object fields
		static ObjectAccessor GetObjectAccessor(void);

		// Create Resources
		static void CreateVertexShader(const wchar_t* file, const char* entry, char layout);
		static void CreatePixelShader(const wchar_t* file, const char* entry);
		static ID3D11Buffer* CreateBuffer(D3D11_BIND_FLAG, void* data, int byte_size);

		// Bind data to the vertex and pixel shaders
		static void BindVSData(unsigned int index, void* data, int byte_size);
		static void BindPSData(unsigned int index, void* data, int byte_size);

	// Private Static Class Methods
	// Helper methods to accomplish the above tasks
	private:
		// Binds Data to a Shader
		static void bind_data(Shader_Type type, unsigned int index, void* data, int byte_size);

	// VisualAttribute Instance
	protected:
		Object* object;

	public:
		VisualAttribute(Object* object);
		~VisualAttribute();

		// Prepare for a draw call
		virtual void prepare(void) = 0;

		// Render an entire scene (call once per frame)
		virtual void render(void) = 0;

		// Finish drawing
		virtual void finish(void) = 0;
	};
}
}