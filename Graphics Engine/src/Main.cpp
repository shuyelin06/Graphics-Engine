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

// Main Engine Inclusions
#include "Main.h" 

#include "objects/Object.h"
#include "objects/other/Camera.h"
#include "objects/renderable/Cube.h"

#include "rendering/buffers/ShaderData.h" // TEMP

// Function Declaration
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

using namespace Engine;

// Major Program Variables
Input::InputEngine input_engine = Input::InputEngine();             // Handles Input
Graphics::VisualEngine graphics_engine = Graphics::VisualEngine();    // Handles Graphics

Datamodel::Cube cube = Datamodel::Cube(2.5f);
Datamodel::Camera camera = Datamodel::Camera(1.56f);

static void rotate(void)
{
    //camera.offsetRoll(0.01f);
    // camera.offsetZ(-0.1f);
    // cube.offsetPitch(0.01f);
}

// Main Function
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{ 
    // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
    input_engine.bindKeyDown(0x57, rotate);

    /* Register a Window Class with the OS */
    // Registers information about the behavior of the application window
    const wchar_t CLASS_NAME[] = L"Application";

    // We fill in a WNDCLASS structure to register a window class
    WNDCLASS wc = { };

    // Required parameters to set prior to registering
    wc.lpfnWndProc = WindowProc; // Function pointer to WindowProc
    wc.hInstance = hInstance; // Handle to this application instance
    wc.lpszClassName = CLASS_NAME; // String identifying the window class

    // Register a window class
    RegisterClass(&wc);

    /* Creates a new Window Instance */
     // Creates the window, and receive a handle uniquely identifying the window (stored in hwnd)
    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Graphics Engine",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    assert(hwnd != NULL); // Check Success

    ShowWindow(hwnd, nCmdShow); // Set Window Visible

    /* Initialize Direct 3D 11 */
    graphics_engine.initialize(hwnd);

    // Create Camera
    camera.setPosition(0, 0, -5);

    // Define Vertex Buffer to Render

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

    bool close = false;
    // Main loop: runs once per frame
    while (!close) {
        /* Handle user input and other window events */
        // TODO: Drain full message loop (in while) until we are done processing all messages
        // Generally, we handle all messages every frame
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (msg.message == WM_QUIT) { break; }

        // Handle user input
        input_engine.handleInput(); 

        // Do other stuff
        Engine::ShaderData shaderData;
        shaderData.color = Engine::Math::Vector3(0.6f, 0.2f, 0.2f);

        cube.offsetPitch(0.01f);
        cube.offsetRoll(0.01f);

        // World to projection
        Matrix4 local_to_world = cube.localToWorldMatrix();
        Matrix4 world_to_camera = camera.localToWorldMatrix().inverse();
        Matrix4 camera_to_project = camera.localToProjectionMatrix();

        // This matrix implementation is column major, so the rightmost matrix is the first transform.
        // However, in D3D, they're row major, so we need to multiply as v * M
        Matrix4 matrix = local_to_world * world_to_camera * camera_to_project;
        graphics_engine.bind_vs_data(0, matrix.getRawData(), sizeof(float) * 16);
        
        // Render
        float color[4] = { 0, 0, 0, 1.0f };
        graphics_engine.clear_screen(color);

        // Draw
        graphics_engine.bind_vertex_shader(0);
        graphics_engine.bind_pixel_shader(0);
        
        graphics_engine.draw(cube.getVertexBuffer());

        graphics_engine.present();
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

    case WM_KEYDOWN:
        camera.setFOV(camera.getFOV() + 0.01f);

        // camera.offsetZ(0.01f);
        // Handle key down input
        return 0;

    case WM_KEYUP:
        // Handle key up input
        return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
