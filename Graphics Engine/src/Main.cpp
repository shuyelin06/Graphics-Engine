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

#include "rendering/VisualEngine.h"

// Function Declaration
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// #define TEST
#ifdef TEST
#include "math/Matrix4.h"
#endif

#include <iostream>
#include "datamodel/shaders/ShaderData.h"

// Main Function
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{ 
#ifdef TEST
    Engine::Math::Matrix4 matrix = Engine::Math::Matrix4(
        1,2,3,4,
        5,2,3,1,
        2,5,5,1,
        1,2,3,6);

    float det = matrix.determinant();
    Engine::Math::Matrix4 inv = matrix.inverse();
    
    float m1 = matrix.minor(0, 0);
    float m2 = matrix.minor(0, 1);
    float m3 = matrix.minor(0, 2);
    float m4 = matrix.minor(0, 3);

    Engine::Math::Matrix4 matrix3;


    /*
    Engine::Matrix3 inv = matrix.inverse();
    Engine::Matrix3 transpose = matrix.transpose();

    Engine::Matrix3 result = matrix * inv;
    */

    return 0;
#else
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
    Engine::VisualEngine visual_engine = Engine::VisualEngine(hwnd);
    visual_engine.initialize();

    // Set the window to be visible
    ShowWindow(hwnd, nCmdShow);

    // Define Vertex Buffer to Render
    float vertex_data_array[] = {
        0.0f,  0.5f,  0.75f, // point at top
       0.5f, -0.5f,  0.0f, // point at bottom-right
      -0.5f, -0.5f,  0.25f, // point at bottom-left

      0.0f,  0.25f,  0.25f, // point at top
       1.0f, -0.25f, 0.25f, // point at bottom-right
      -0.25f, -0.25f, 0.25f, // point at bottom-left
    };

    Engine::VertexBuffer buffer;
    buffer.vertices = vertex_data_array;
    buffer.num_vertices = 6;
    buffer.vertex_size = 3;

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

        // Render
        float color[4] = { 0x64 / 255.0f, 0x95 / 255.0f, 0xED / 255.0f, 1.0f };
        visual_engine.clear_screen(color);

        // Draw
        visual_engine.bind_vertex_shader(0);
        visual_engine.bind_pixel_shader(0);
        
        visual_engine.draw(buffer);

        visual_engine.present();
    }
#endif

    // Finish
    return 0;
}

/*
Engine::ShaderData shaderData;
shaderData.color = Engine::Math::Vector3(0.75f, 0.2f, 0.2f);

// Fill buffer description
D3D11_BUFFER_DESC cDesc;
cDesc.ByteWidth = sizeof(Engine::ShaderData);
cDesc.Usage = D3D11_USAGE_DYNAMIC;
cDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
cDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
cDesc.MiscFlags = 0;
cDesc.StructureByteStride = 0;

ID3D11Buffer* g_pConstantBuffer11 = NULL;

// Fill subresource data
D3D11_SUBRESOURCE_DATA InitData;
InitData.pSysMem = &shaderData;
InitData.SysMemPitch = 0;
InitData.SysMemSlicePitch = 0;

HRESULT result = device->CreateBuffer(
    &cDesc, &InitData, &g_pConstantBuffer11
);

assert(SUCCEEDED(result));

device_context->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);
*/

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
