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

#include "rendering/VisualEngine.h"
#include "input/InputEngine.h"

#include "datamodel/shaders/ShaderData.h" // TEMP

// Function Declaration
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

using namespace Engine;

Input::InputEngine input_engine = Input::InputEngine();

int flag = 0;

static void setTrue(void)
{
    flag = 1;
}

static void setFalse(void)
{
    flag = 0;
}


// Main Function
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{ 
    input_engine.bindKeyDown(0x57, setTrue); // TEMP
    input_engine.bindKeyUp(0x57, setFalse);

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
    Graphics::VisualEngine visual_engine = Graphics::VisualEngine(hwnd);
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

    Graphics::VertexBuffer buffer;
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

        // Handle user input
        input_engine.handleInput(); 
        
        // Do other stuff
        float c = 0.15f;
        if (flag)
        {
            c = 0.75f;
        }

        Engine::ShaderData shaderData;
        shaderData.color = Engine::Math::Vector3(c, 0.2f, 0.2f);

        visual_engine.bind_data(Graphics::Pixel, 0, &shaderData, sizeof(shaderData));

        // Render
        float color[4] = { 0x64 / 255.0f, 0x95 / 255.0f, 0xED / 255.0f, 1.0f };
        visual_engine.clear_screen(color);

        // Draw
        visual_engine.bind_vertex_shader(0);
        visual_engine.bind_pixel_shader(0);
        
        visual_engine.draw(buffer);

        visual_engine.present();
    }

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

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
