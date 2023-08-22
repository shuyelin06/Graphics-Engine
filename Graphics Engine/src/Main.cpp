// Ensure that the UNICODE symbol is defined
#ifndef UNICODE
#define UNICODE
#endif 

// Assertions
#include <assert.h>

// Win32 Library Include
#include <windows.h>
// Direct 3D 11 Library Includes
#include <d3d11.h> // Direct 3D Interface
#include <dxgi.h> // DirectX Driver Interface
#include <d3dcompiler.h> // Shader Compiler

// Indicates Visual C++ to leave a command in the object file, which can be read by
// the linker when it processes object files.
// Tells the linker to add the "library" library to the list of library dependencies
#pragma comment( lib, "user32" )          // link against the win32 library
#pragma comment( lib, "d3d11.lib" )       // direct3D library
#pragma comment( lib, "dxgi.lib" )        // directx graphics interface
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler

#include <math.h>

// Function Declaration
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define TEST
#ifdef TEST
#include "datamodel/geometry/Matrix3.h"
#endif

// Main Function
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{ 
#ifdef TEST
    Engine::Matrix3 matrix = Engine::Matrix3(1,2,3,3,2,1,2,1,3);

    Engine::Matrix3 inv = matrix.inverse();
    Engine::Matrix3 transpose = matrix.transpose();

    Engine::Matrix3 result = matrix * inv;

    return 0;
#else
    /* Direct3 Pointers */
    ID3D11Device* device_ptr = NULL;
    ID3D11DeviceContext* device_context_ptr = NULL;
    IDXGISwapChain* swap_chain_ptr = NULL;
    ID3D11RenderTargetView* render_target_view_ptr = NULL;

    /* Registers a Window Class with the Operating System
     * A window clas defines a set of behaviors for a window to inherit.
     * Windows inheriting the same class wil have similar behavior (though not completely
     * identical, due to instance data).
    */
    // Registers information about the behavior of the application window
    const wchar_t CLASS_NAME[] = L"Hello Triangle!";

    // We fill in a WNDCLASS structure to register a window class
    WNDCLASS wc = { };

    // Required parameters to set prior to registering
    wc.lpfnWndProc = WindowProc; // Function pointer to WindowProc
    wc.hInstance = hInstance; // Handle to this application instance
    wc.lpszClassName = CLASS_NAME; // String identifying the window class

    // Register a window class
    RegisterClass(&wc);

    /* Creates a new Window instance from the class
     */
     // Creates the window, and receive a handle uniquely identifying the window (stored in hwnd)
    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Learn to Program Windows",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    // Check if creation was successful
    if (hwnd == NULL) {
        return 0;
    }

    /* Initialize Direct 3D 11 */
    // Populate a Swap Chain Structure, responsible for swapping between textures (for rendering)
    DXGI_SWAP_CHAIN_DESC swap_chain_descriptor = { 0 };
    swap_chain_descriptor.BufferDesc.RefreshRate.Numerator = 0; // Synchronize Output Frame Rate
    swap_chain_descriptor.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_descriptor.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // Color Output Format
    swap_chain_descriptor.SampleDesc.Count = 1;
    swap_chain_descriptor.SampleDesc.Quality = 0;
    swap_chain_descriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_descriptor.BufferCount = 1; // Number of back buffers to add to the swap chain
    swap_chain_descriptor.OutputWindow = hwnd;
    swap_chain_descriptor.Windowed = true; // Displaying to a Window

    // Create swap chain, and save pointer data
    D3D_FEATURE_LEVEL feature_level;
    UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined( DEBUG ) || defined( _DEBUG )
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    // Create Device and Swap Chain using our created struct
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        flags,
        NULL,
        0,
        D3D11_SDK_VERSION,
        &swap_chain_descriptor,
        &swap_chain_ptr,
        &device_ptr,
        &feature_level,
        &device_context_ptr);
    // Check for success
    assert(S_OK == hr && swap_chain_ptr && device_ptr && device_context_ptr);

    // Obtain the Render Targets (Output Images)
    // Obtain Frame Buffer
    ID3D11Texture2D* framebuffer;
    hr = swap_chain_ptr->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        (void**)&framebuffer);
    // Check if we've successfully obtained the frame buffer 
    assert(SUCCEEDED(hr));

    // Create Render Target with Frame Buffer
    hr = device_ptr->CreateRenderTargetView(
        framebuffer, 0, &render_target_view_ptr);
    // Check Success
    assert(SUCCEEDED(hr));

    framebuffer->Release();

    /* Build our Shaders using D3DCompileFromFile() */
    // D3DCompileFromFile() returns a binary object for each shader, known as a blob
    flags = D3DCOMPILE_ENABLE_STRICTNESS; // Set flags
#if defined(DEBUG) || defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG; // More Debug Output
#endif
    // Initilaize pointers for the vertex shader, pointer shader, and error blobs
    ID3DBlob* vs_blob_ptr = NULL, * ps_blob_ptr = NULL, * error_blob = NULL;
    
    // Compile Vertex Shader
    hr = D3DCompileFromFile(
        L"src/shaders/shader.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "vs_main", // Entry Point
        "vs_5_0", // Vertex Shader Compiler Target
        flags, // Flags
        0,
        &vs_blob_ptr,
        &error_blob);
    // Check for Success
    if (FAILED(hr)) {
        // Check Error Blob
        if (error_blob) {
            // Output Debug String
            OutputDebugStringA((char*)error_blob->GetBufferPointer());
            // Release (Free) Error Blob
            error_blob->Release();
        }
        // Release the Vertex Shader Blob if Allocated
        if (vs_blob_ptr) { vs_blob_ptr->Release(); }
        assert(false);
    }

    // Compile Pixel Shader
    hr = D3DCompileFromFile(
        L"src/shaders/shader.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "ps_main", // Entry Point
        "ps_5_0", // PixelShader Compiler Target
        flags,
        0,
        &ps_blob_ptr,
        &error_blob);
    // Check for Success
    if (FAILED(hr)) {
        // Check Error Blob
        if (error_blob) {
            // Output Debug String
            OutputDebugStringA((char*)error_blob->GetBufferPointer());
            // Release Error Blob
            error_blob->Release();
        }
        // Release the Pixel Shader Blob if Allocated
        if (ps_blob_ptr) { ps_blob_ptr->Release(); }
        assert(false);
    }

    /* Create Shaders from our Compiled Shader Blobs */
    ID3D11VertexShader* vertex_shader_ptr = NULL;
    ID3D11PixelShader* pixel_shader_ptr = NULL;

    // Create Vertex Shader
    hr = device_ptr->CreateVertexShader(
        vs_blob_ptr->GetBufferPointer(),
        vs_blob_ptr->GetBufferSize(),
        NULL,
        &vertex_shader_ptr);
    assert(SUCCEEDED(hr));

    // Create Pixel Shader
    hr = device_ptr->CreatePixelShader(
        ps_blob_ptr->GetBufferPointer(),
        ps_blob_ptr->GetBufferSize(),
        NULL,
        &pixel_shader_ptr);
    assert(SUCCEEDED(hr));

    /* Create Input Layout */
    // This describes how vertex data in memory will map to input
    // variables for the vertex shader!
    // This is done by filling out an array of D3D11_INPUT_ELEMENT_DESC structs, and
    // passing that array to CreateInputLayout()
    ID3D11InputLayout* input_layout_ptr = NULL;

    // Create Array of INPUT_ELEMENT_DESC
    // The vertex shader only takes position so far, so we only pass in position
    // This is an XYZ (float3) coordinate, where each element is a 32-bit float.
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        // "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        /* { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 } */
        // The D3D11_APPEND_ALIGNED_ELEMENT macro specifies that the element direct follows the previous one in memory 
        // (in this case, COLOR would start at the 4th float).
    };

    // Create Input Layout for our Vertex Shader
    hr = device_ptr->CreateInputLayout(
        inputElementDesc,
        ARRAYSIZE(inputElementDesc),
        vs_blob_ptr->GetBufferPointer(),
        vs_blob_ptr->GetBufferSize(),
        &input_layout_ptr);
    assert(SUCCEEDED(hr));

    /* Define Vertex Points to Put into our Vertex Shader */
    // This is simply an array of floats - but given our specified input layout,
    // this will be interpreted as an input of float3 types!
    float vertex_data_array[] = {
        0.0f,  0.5f,  0.75f, // point at top
       0.5f, -0.5f,  0.0f, // point at bottom-right
      -0.5f, -0.5f,  0.25f, // point at bottom-left

      0.0f,  0.25f,  0.25f, // point at top
       1.0f, -0.25f, 0.25f, // point at bottom-right
      -0.25f, -0.25f, 0.25f, // point at bottom-left
    };
    UINT vertex_stride = 3 * sizeof(float); // Bytes between the beginning of each vertex
    UINT vertex_offset = 0; // Offset into the buffer to start reading
    UINT vertex_count = 6; // Total number of vertices

    // Load this array into the vertex buffer
    ID3D11Buffer* vertex_buffer_ptr = NULL; // Create a vertex buffer
    {
        // Descriptor for Vertex buffer
        D3D11_BUFFER_DESC vertex_buff_descr = {};
        vertex_buff_descr.ByteWidth = sizeof(vertex_data_array); // Size of buffer
        vertex_buff_descr.Usage = D3D11_USAGE_DEFAULT; // Usage of buffer (helps with driver optimization)
        vertex_buff_descr.BindFlags = D3D11_BIND_VERTEX_BUFFER; // Vertex Buffer type

        // Subresource Data?
        // Load Data
        D3D11_SUBRESOURCE_DATA sr_data = { 0 };
        sr_data.pSysMem = vertex_data_array;

        // Create Vertex Buffer with data loaded into it
        HRESULT hr = device_ptr->CreateBuffer(
            &vertex_buff_descr,
            &sr_data,
            &vertex_buffer_ptr);
        assert(SUCCEEDED(hr));
    }

    // Set the window to be visible
    ShowWindow(hwnd, nCmdShow);

    /*
    * Begin a message loop until the user closes the windowand exits the application
    *
    * Windows communicates with the program by passing it a series of messages (seen in this
    * loop). Every time the DispatchMessage() function is called, the WindowProc function for the window is
    * indirectly invoked as well!
    */
    MSG msg = { };

    /* Normal Win32 Message Loop
    // GetMessage() is blocking, so main will wait for any event to occur (key press, mouse click)
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    */

    float theta = 0.0f;

    bool close = false;
    while (!close) {
        /* Handle user input and other window events */
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (msg.message == WM_QUIT) { break; }



        vertex_data_array[0] = sin(theta);
        theta += 0.01;

        /* Handle Rendering with Direct3D */
        // Clear back buffer (and depth buffer if available)
        float background_color[4] = {
             0x64 / 255.0f, 0x95 / 255.0f, 0xED / 255.0f, 1.0f };
        device_context_ptr->ClearRenderTargetView(
            render_target_view_ptr, background_color);

        // Set the valid drawing area (our window)
        RECT winRect;
        GetClientRect(hwnd, &winRect); // Get the windows rectangle

        D3D11_VIEWPORT viewport = {
            0.0f, 0.0f,
            (FLOAT)(winRect.right - winRect.left),
            (FLOAT)(winRect.bottom - winRect.top),
            0.0f,
            1.0f
        };
        // Give rectangle to rasterizer state function
        device_context_ptr->RSSetViewports(1, &viewport);

        // Set output merger to use our render target
        device_context_ptr->OMSetRenderTargets(1, &render_target_view_ptr, NULL);

        // Set the Input Assembler to use our vertex buffer
        // Triangle list - every 3 vertices forms a separate triangle
        device_context_ptr->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // Set primitive format 
        device_context_ptr->IASetInputLayout(input_layout_ptr); // Set input layout
        device_context_ptr->IASetVertexBuffers( // Use the vertex buffer we created!
            0,
            1,
            &vertex_buffer_ptr,
            &vertex_stride,
            &vertex_offset
        );

        // Set the Vertex Shader
        device_context_ptr->VSSetShader(vertex_shader_ptr, NULL, 0);

        // Set the pixel shader
        device_context_ptr->PSSetShader(pixel_shader_ptr, NULL, 0);

        // Draw 3 vertices from our vertex buffer
        // Draw will use all of the states we set, the vertex buffer and shaders.
        // We specify how many vertices to draw from our vertex buffer.
        device_context_ptr->Draw(vertex_count, 0);

        // Now that this has been rendered, we use the swap chain and swap the buffers, to show the screen
        swap_chain_ptr->Present(1, 0);
    }
#endif

    // Finish
    return 0;
}

// Defines the behavior of the window (appearance, user interaction, etc)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // All painting occurs here, between BeginPaint and EndPaint.

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);
    }
    return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
