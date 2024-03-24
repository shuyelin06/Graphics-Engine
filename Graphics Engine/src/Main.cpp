// Ensure that the UNICODE symbol is defined
#ifndef UNICODE
#define UNICODE
#endif 

// Assertions
#include <assert.h>

// Win32 Library Include
#include <windows.h>
#include <WindowsX.h> // Input Macros

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
#include "objects/other/Player.h"	// Main Player
#include "rendering/VisualEngine.h" // Graphics Engine
#include "input/InputEngine.h"		// Input Engine

#include "objects/Object.h"
#include "rendering/Mesh.h"

// TEST
#include "utility/Stopwatch.h"

// Function Declaration
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

using namespace std;
using namespace Engine;

// Major Program Variables
static Datamodel::Player player = Datamodel::Player();                       // Player
static Input::InputEngine input_engine = Input::InputEngine();               // Handles Input
static Graphics::VisualEngine graphics_engine = Graphics::VisualEngine();    // Handles Graphics

// Main Function
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
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

    // Set screen center
    {
        RECT window_rect;
        GetWindowRect(hwnd, &window_rect);

        int center_x = (window_rect.right - window_rect.left) / 2;
        int center_y = (window_rect.bottom - window_rect.top) / 2;
        input_engine.setScreenCenter(center_x, center_y);
    }


    /* Initialize Direct 3D 11 */
    graphics_engine.initialize(hwnd);

    // TESTING
    Graphics::Mesh mesh = Graphics::Mesh::parsePLYFile("data/Beethoven.ply");
    mesh.setShaders(0, 0);
    mesh.calculateNormals();

    Datamodel::Object cube = Datamodel::Object();
    cube.setMesh(&mesh);
    Datamodel::Object cube2 = Datamodel::Object();
    cube2.setMesh(&mesh);
    cube2.offsetPosition(0, 0, -10);

    // Adjust
    player.setPosition(0, 0, -5);

   

    // Define Vertex Buffer to Render

    /*
    * Begin a message loop until the user closes the windowand exits the application
    *
    * Windows communicates with the program by passing it a series of messages (seen in this
    * loop). Every time the DispatchMessage() function is called, the WindowProc function for the window is
    * indirectly invoked as well!
    */
    MSG msg = { };

    bool close = false;

    // Create vector of renderable objects
    std::vector<Datamodel::Object*> objects;
    objects.push_back(&cube);
    objects.push_back(&cube2);

    // Create timer watches
    Utility::Stopwatch framerate_watch = Utility::Stopwatch();
    Utility::Stopwatch physics_watch = Utility::Stopwatch();

    // Main loop: runs once per frame
    while (!close) {
        // Begin counting milliseconds elapsed for framerate
        framerate_watch.Reset();

        // Handle Input
        {
            // Drain and process all queued messages
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                if (msg.message == WM_QUIT)
                    return 0;
            }

            // Handle mouse x camera movement 
            input_engine.updateCameraView(player.getCamera());
        }
        
        // Handle Physics
        {
            // Determine time elapsed for physics updating
            double time_elapsed = physics_watch.Duration();

            cube.offsetRotation(0, 0.01f, 0);
            cube2.offsetRotation(0, 0.01f, 0);

            player.physicsUpdate(time_elapsed);

            // Reset watch
            physics_watch.Reset();
        }
        
        // Handle Rendering
        {
            // Prepare for drawing
            graphics_engine.prepare();

            // Render all objects
            for (int i = 0; i < objects.size(); i++) 
            {
                Datamodel::Object* o = objects[i];
                graphics_engine.drawObject(player.getCamera(), o);
            }

            // Present to screen
            graphics_engine.present();
        }        

        // Stall until enough time has elapsed for 60 frames / second
        while (framerate_watch.Duration() < 1 / 60.f) {}
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
    {
        ClipCursor(NULL);
        PostQuitMessage(0);
    }
    return 0;
    
    // Key Down
    case WM_KEYDOWN:
    {
        int keycode = wParam;
        input_engine.handleKeyDown(&player, keycode);
    }
    break;

    // Key Up
    case WM_KEYUP:
    {
        int keycode = wParam;

        input_engine.handleKeyUp(keycode);
    }
    break;


    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
