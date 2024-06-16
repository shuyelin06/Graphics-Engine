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

#include <stdlib.h>
#include <time.h>

// Main Engine Inclusions
#include "simulation/SimulationEngine.h"  // Simulation Engine
#include "rendering/VisualEngine.h" // Graphics Engine
#include "input/InputEngine.h"		// Input Engine

#include "datamodel/Scene.h"

#include "math/Compute.h"

// TEST
#include "utility/Stopwatch.h"

// Function Declaration
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

using namespace std;

using namespace Engine;

using namespace Engine::Simulation;
using namespace Engine::Datamodel;
using namespace Engine::Input;
using namespace Engine::Graphics;

static InputEngine input_engine = InputEngine();

static Scene* _scene;
static Terrain* terrain;
static int config;

static bool TestFunction(InputData data)
{
    if (data.input_type == SYMBOL_DOWN)
    {
        return true;
    }
    return false;
}

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

    /* Create and Initialize Engines */
    VisualEngine graphics_engine = VisualEngine();
    graphics_engine.initialize(hwnd);

    /* Create and Initialize Physics Engine */
    SimulationEngine simulation_engine = SimulationEngine();
    simulation_engine.initalize();

    // Set screen center
    {
        RECT window_rect;
        GetWindowRect(hwnd, &window_rect);

        int center_x = (window_rect.right - window_rect.left) / 2;
        int center_y = (window_rect.bottom - window_rect.top) / 2;
        input_engine.setScreenCenter(center_x, center_y);
    }

    /* Seed Random Number Generator */
    srand(0);
    // time(NULL)

    /* Load All Meshes */
    Mesh::LoadMeshes();

    /* Initialize Scene */
    Scene scene = Scene();
    input_engine.setScene(&scene);
    terrain = &(scene.getTerrain());
    _scene = &scene;

    // Create Objects


    /*
    Object& parent = scene.createObject();
    parent.setMesh(Mesh::GetMesh("Cube"));
    parent.getTransform().setScale(30, 2, 30);
    */

    // Create Lights
    const int NUM_LIGHTS = 5;

    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        Light& light = scene.createLight();
        Transform& transform = light.getTransform();

        light.setMesh(Mesh::GetMesh("Cube"));
        transform.setPosition(Compute::random(-20, 20), Compute::random(-20, 20), Compute::random(-20, 20));
    }

    // Begin window messaging loop
    MSG msg = { };
    bool close = false;

    Utility::Stopwatch framerate_watch = Utility::Stopwatch();
    Utility::Stopwatch physics_watch = Utility::Stopwatch();

    // Main loop: runs once per frame
    while (!close) {
        // Begin counting milliseconds elapsed for framerate
        framerate_watch.Reset();

        // --- Process Input Data ---
        // Drain and process all queued messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                return 0;
        }

        // Dispatch accumulated input data
        input_engine.dispatch();

        // Handle mouse x camera movement 
        // TODO: Integrate this with the existing input pipeline
        input_engine.updateCameraView();

        // --- Object Behavioral Update ---
        scene.getCamera().update();
        for (Object* object : scene.getObjects())
            object->update();

        // --- Physics Pass ---
        
        // Handle Physics
        {
            // Determine time elapsed for physics updating
            double time_elapsed = physics_watch.Duration();

            vector<Object*>& objects = scene.getObjects();

            for (int i = 0; i < objects.size(); i++)
            {
                Object* o = objects[i];

                Transform& transform = o->getTransform();
                Vector3& velocity = o->getVelocity();
                Vector3& acceleration = o->getAcceleration();

                transform.offsetRotation(0, 0.01f, 0);

                // Update Velocity
                velocity += acceleration * time_elapsed;

                // Update Position
                transform.offsetPosition(velocity.x * time_elapsed, velocity.y * time_elapsed, velocity.z * time_elapsed);

                // Dampen Velocity
                Vector3 newVelocity = velocity * 0.85f;
                velocity.x = newVelocity.x;
                velocity.y = newVelocity.y;
                velocity.z = newVelocity.z;
            }

            Vector3& velocity = scene.getCamera().getVelocity();
            Vector3& acceleration = scene.getCamera().getAcceleration();

            // Update Velocity
            velocity += acceleration * time_elapsed;

            // Update Position
            scene.getCamera().getTransform().offsetPosition(velocity.x * time_elapsed, velocity.y * time_elapsed, velocity.z * time_elapsed);

            // Dampen Velocity
            Vector3 newVelocity = velocity * 0.85f;
            velocity.x = newVelocity.x;
            velocity.y = newVelocity.y;
            velocity.z = newVelocity.z;

            // Reset watch
            physics_watch.Reset();
        }

        // Handle Rendering
        graphics_engine.render(scene);

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
    // Destroys the Application on any Force Quit or Escape
    case WM_DESTROY:
    {
        ClipCursor(NULL);
        PostQuitMessage(0);
    }
    return 0;

    
    // Key Down
    case WM_KEYDOWN:
    {
        // Escape will always quit the application, just in case
        if (wParam == VK_ESCAPE)
        {
            ClipCursor(NULL);
            PostQuitMessage(0);
            return 0;
        }
        input_engine.logWin32Input(uMsg, wParam);
    }
    break;

    // Key Up
    case WM_KEYUP:
        input_engine.logWin32Input(uMsg, wParam);
    break;


    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
