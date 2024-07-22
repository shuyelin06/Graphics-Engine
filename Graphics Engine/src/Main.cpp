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

#include "simulation/PhysicsSystem.h"
#include "rendering/VisualSystem.h"
#include "input/InputSystem.h"

#include "rendering/VisualDebug.h"

#include "rendering/AssetManager.h"

#include "math/Compute.h"
#include "utility/Stopwatch.h"

using namespace Engine;
using namespace Engine::Simulation;
using namespace Engine::Datamodel;
using namespace Engine::Input;
using namespace Engine::Graphics;

// Macro for creating new components
#define NEW_COMPONENT(System, ComponentType) System.ComponentHandler<ComponentType>::createComponent()

// Handles windows messages, including input.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Updates all object transforms and caches their matrices prior to any
// physics or render calls.
void UpdateObjectTransforms(Object* object, const Matrix4& m_parent);

// Static reference to the input system for use in the window message callback
static InputSystem input_system;

#include "rendering/Shader.h"

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

    // Seed Random Number Generator
    srand(0);

    // Create Input System
    input_system = InputSystem();
    input_system.initialize();

    // Create Visual System
    VisualSystem visual_system = VisualSystem(hwnd);
    visual_system.initialize();

    // Create Physics System
    PhysicsSystem physics_system = PhysicsSystem();
    physics_system.initialize();

    // Create Object Hierarchy
    // TODO:
    Object* parent_object = new Object();

    // visual_system.bindMeshComponent(&light)->setMesh(Mesh::GetMesh("Cube"));
    
    // Create a camera
    Object& camera = parent_object->createChild();
    camera.getTransform().setPosition(0, 0, -25);

    visual_system.bindViewComponent(&camera);
    input_system.bindMovementComponent(&camera);
    // visual_system.bindLightComponent(&camera);
    // MeshComponent* mesh = visual_system.bindMeshComponent(&camera);
    // mesh->setMesh(Mesh::GetMesh("Cube"));
    
    {
        Object& child = parent_object->createChild();
        child.getTransform().setScale(5, 5, 5);
        child.getTransform().setPosition(Compute::random(-2.5f, 2.5f), Compute::random(-2.5f, 2.5f), Compute::random(15, 25));
        
        visual_system.bindAssetComponent(&child, Fox);
    }

    Object& light = parent_object->createChild();
    // light.getTransform().offsetRotation(-0.05f, 0, 0);
    visual_system.bindLightComponent(&light);

    {
        Object& child = parent_object->createChild();
        child.getTransform().setScale(100, 2.5f, 100);
        child.getTransform().setPosition(0, -10, 0);

        visual_system.bindAssetComponent(&child, Cube);
    }

    
    // Begin window messaging loop
    MSG msg = { };
    bool close = false;

    Utility::Stopwatch framerate_watch = Utility::Stopwatch();

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

        // Draw XYZ Axes (R: +X, G: +Y, B: +Z)
        VisualDebug::DrawPoint(Vector3(0, 0, 0), 1, Color::White());
        VisualDebug::DrawPoint(Vector3(1, 0, 0), 1, Color::Red());
        VisualDebug::DrawPoint(Vector3(0, 1, 0), 1, Color::Green());
        VisualDebug::DrawPoint(Vector3(0, 0, 1), 1, Color::Blue());

        VisualDebug::DrawLine(Vector3(0, 0, 0), Vector3(5, 0, 0), Color::Red());
        VisualDebug::DrawLine(Vector3(0, 0, 0), Vector3(0, 5, 0), Color::Green());
        VisualDebug::DrawLine(Vector3(0, 0, 0), Vector3(0, 0, 5), Color::Blue());

        light.getTransform().offsetRotation(0.01f, 0, 0);

        // Update Object Transforms
        Matrix4 identity = Matrix4::identity();
        UpdateObjectTransforms(parent_object, identity);
        
        // Dispatch Input Data
        input_system.update();

        // Update Physics System
        physics_system.update();
        
        // Update Rendering System
        visual_system.update();

        // Stall until enough time has elapsed for 60 frames / second
        while (framerate_watch.Duration() < 1 / 60.f) {}
    }

    /*
    * 
        // Handle mouse x camera movement 
        // TODO: Integrate this with the existing input pipeline
        input_system.updateCameraView();


    // Set screen center
    {
        RECT window_rect;
        GetWindowRect(hwnd, &window_rect);

        int center_x = (window_rect.right - window_rect.left) / 2;
        int center_y = (window_rect.bottom - window_rect.top) / 2;
        input_system.setScreenCenter(center_x, center_y);
    }




    // Create Lights
    const int NUM_LIGHTS = 5;

    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        Light& light = scene.createLight();
        Transform& transform = light.getTransform();

        light.setMesh(Mesh::GetMesh("Cube"));
        transform.setPosition(Compute::random(-20, 20), Compute::random(-20, 20), Compute::random(-20, 20));
    }
    */

    // Finish
    return 0;
}

// UpdateObjectTransforms:
// Recursively traverses a scene hierarchy and updates the Local -> World
// matrix for every object. Does this efficiently by using the matrix
// for each object's parent. 
void UpdateObjectTransforms(Object* object, const Matrix4& m_parent)
{
    // Object pointer should never be a null pointer.
    if (object == nullptr)
        assert(false);

    // Update the object local matrix, and save it for the recursive call
    // on its (potential) children.
    Matrix4 m_local = object->updateLocalMatrix(m_parent);

    for (Object* child : object->getChildren())
        UpdateObjectTransforms(child, m_local);
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
        input_system.logWin32Input(uMsg, wParam);
    }
    break;

    // Key Up
    case WM_KEYUP:
        input_system.logWin32Input(uMsg, wParam);
    break;


    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
