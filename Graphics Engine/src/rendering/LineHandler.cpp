#include "VisualAttribute.h"

namespace Engine
{
namespace Graphics
{
	// Line buffer
	static vector<float> lines = vector<float>();
	
    // Line handler shaders
    static int v_shader = -1;
    static int p_shader = -1;

	// D3D11 handle for rendering
    static Matrix4 camera_matrix = Matrix4();
    static ID3D11Buffer* line_buffer = NULL;

    // DrawLine:
    // Queues a line to be drawn
    void VisualAttribute::DrawLine(Vector3 p1, Vector3 p2, Vector3 rgb)
    {
        lines.push_back(p1.x);
        lines.push_back(p1.y);
        lines.push_back(p1.z);
        lines.push_back(rgb.x);
        lines.push_back(rgb.y);
        lines.push_back(rgb.z);
        lines.push_back(p2.x);
        lines.push_back(p2.y);
        lines.push_back(p2.z);
        lines.push_back(rgb.x);
        lines.push_back(rgb.y);
        lines.push_back(rgb.z);
        lines.push_back(p2.x);
        lines.push_back(p2.y);
        lines.push_back(p2.z);
    }

    // Initialize Line Handler
    void VisualAttribute::InitializeLineHandler()
    {
        v_shader = CreateVertexShader(L"src/shaders/LineHandler.hlsl", "vs_main", XYZ | RGB);
        p_shader = CreatePixelShader(L"src/shaders/LineHandler.hlsl", "ps_main");
    }

    // PrepareLines:
    // Prepares lines to be rendered
    void VisualAttribute::PrepareLines()
    {
        if (lines.size() == 0)
            return;

        camera_matrix = camera->localToProjectionMatrix();

        line_buffer = CreateBuffer(
            D3D11_BIND_VERTEX_BUFFER,
            (void *) lines.data(),
            sizeof(float) * lines.size());
    }
    
    // RenderLines:
    // Render available lines
    void VisualAttribute::RenderLines()
    {
        if (lines.size() == 0)
            return;

        UINT vertex_stride = VertexLayoutSize(XYZ | RGB) * sizeof(float);
        UINT vertex_offset = 0;
        UINT vertex_count = lines.size();

        BindVSData(0, camera_matrix.getRawData(), sizeof(float) * 16);

        ID3D11InputLayout* input_layout = input_layouts[XYZ | RGB];
        ID3D11VertexShader* vertex_shader = vertex_shaders[v_shader];
        ID3D11PixelShader* pixel_shader = pixel_shaders[p_shader];

        // Perform a Draw Call
        // Set the valid drawing area (our window)
        RECT winRect;
        GetClientRect(window, &winRect); // Get the windows rectangle

        D3D11_VIEWPORT viewport = {
            0.0f, 0.0f,
            (FLOAT)(winRect.right - winRect.left),
            (FLOAT)(winRect.bottom - winRect.top),
            0.0f,
            1.0f
        };

        // Set min and max depth for viewpoint (for depth testing)
        viewport.MinDepth = 0;
        viewport.MaxDepth = 1;

        // Give rectangle to rasterizer state function
        device_context->RSSetViewports(1, &viewport);

        // Set output merger to use our render target and depth test
        device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil);

        /* Configure Input Assembler */
        // Define input layout
        device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        device_context->IASetInputLayout(input_layout);

        // Bind vertex buffers
        device_context->IASetVertexBuffers(0, 1, &line_buffer, &vertex_stride, &vertex_offset);

        /* Configure Shaders*/
        // Bind vertex shader
        device_context->VSSetShader(vertex_shader, NULL, 0);

        // Bind pixel shader
        device_context->PSSetShader(pixel_shader, NULL, 0);

        // Draw from our vertex buffer
        device_context->Draw(vertex_count, 0);

        lines.clear();
    }
}
}