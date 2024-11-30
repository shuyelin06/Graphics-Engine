// Ensure that the UNICODE symbol is defined
#ifndef UNICODE
#define UNICODE
#endif

// Assertions
#include <assert.h>

// Win32 Library Include
#include <WindowsX.h> // Input Macros
#include <windows.h>

// Direct 3D 11 Library Includes
#include <d3d11.h>       // Direct 3D Interface
#include <d3dcompiler.h> // Shader Compiler
#include <dxgi.h>        // DirectX Driver Interface

// Indicates Visual C++ to leave a command in the object file, which can be read
// by the linker when it processes object files. Tells the linker to add the
// "library" library to the list of library dependencies
#pragma comment(lib, "user32")          // link against the win32 library
#pragma comment(lib, "d3d11.lib")       // direct3D library
#pragma comment(lib, "dxgi.lib")        // directx graphics interface
#pragma comment(lib, "d3dcompiler.lib") // shader compiler

#include <stdlib.h>
#include <time.h>

#include "datamodel/SceneGraph.h"
#include "input/InputSystem.h"
#include "rendering/VisualSystem.h"
#include "simulation/PhysicsSystem.h"

#include "input/components/MovementHandler.h"

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
#define NEW_COMPONENT(System, ComponentType)                                   \
    System.ComponentHandler<ComponentType>::createComponent()

// Handles windows messages, including input.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Static reference to the input system for use in the window message callback
static InputSystem input_system;

#include "rendering/Shader.h"
#include "utility/FileReader.h"

// Main Function
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR pCmdLine, int nCmdShow) {
    /* Register a Window Class with the OS */
    // Registers information about the behavior of the application window
    const wchar_t CLASS_NAME[] = L"Application";

    // We fill in a WNDCLASS structure to register a window class
    WNDCLASS wc = {};

    // Required parameters to set prior to registering
    wc.lpfnWndProc = WindowProc;   // Function pointer to WindowProc
    wc.hInstance = hInstance;      // Handle to this application instance
    wc.lpszClassName = CLASS_NAME; // String identifying the window class

    // Register a window class
    RegisterClass(&wc);

    /* Creates a new Window Instance */
    // Creates the window, and receive a handle uniquely identifying the window
    // (stored in hwnd)
    HWND hwnd = CreateWindowEx(0,                   // Optional window styles.
                               CLASS_NAME,          // Window class
                               L"Graphics Engine",  // Window text
                               WS_OVERLAPPEDWINDOW, // Window style

                               // Size and position
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               CW_USEDEFAULT,

                               NULL,      // Parent window
                               NULL,      // Menu
                               hInstance, // Instance handle
                               NULL       // Additional application data
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

    // Create SceneGraph
    SceneGraph scene_graph = SceneGraph();

    // Bind Camera
    MovementHandler movementHandler(visual_system.getCamera().getTransform());

    // Create Object Hierarchy
    Object& parent_object = scene_graph.createObject();
        
    Object& child1 = parent_object.createChild();
        
    Object& child2 = parent_object.createChild();
    child2.getTransform().setScale(5, 5, 5);
    child2.getTransform().setPosition(Compute::random(-2.5f, 2.5f),
                                        Compute::random(-2.5f, 2.5f),
                                        Compute::random(15, 25));

    {
        Object& light = parent_object.createChild();
        light.getTransform().offsetPosition(0, 5, 0);
        light.getTransform().offsetRotation(Vector3::PositiveX(), 0.05f);

        Light* lObj = visual_system.createLight();
        lObj->setTransform(&light.getTransform());
    }

    Object& child3 = parent_object.createChild();
    child3.getTransform().setScale(100, 2.5f, 100);
    child3.getTransform().setPosition(0, -10, 0);

    // Begin window messaging loop
    MSG msg = {};
    bool close = false;

    Utility::Stopwatch framerate_watch = Utility::Stopwatch();

    VisualDebug::DrawPoint(Vector3(0, 0, 0), 1, Color::White(), 60 * 7);
    VisualDebug::DrawPoint(Vector3(1, 0, 0), 1, Color::Red(), 60 * 7);
    VisualDebug::DrawPoint(Vector3(0, 1, 0), 1, Color::Green(), 60 * 7);
    VisualDebug::DrawPoint(Vector3(0, 0, 1), 1, Color::Blue(), 60 * 7);

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

        // VisualDebug::DrawLine(Vector3(0, 0, 0), Vector3(5, 0, 0),
        // Color::Red()); VisualDebug::DrawLine(Vector3(0, 0, 0), Vector3(0, 5,
        // 0), Color::Green()); VisualDebug::DrawLine(Vector3(0, 0, 0),
        // Vector3(0, 0, 5), Color::Blue());

        // Update Object Transforms
        scene_graph.updateObjectTransforms();

        // Dispatch Input Data
        movementHandler.update();
        input_system.update();

        // Update Physics System
        physics_system.update();

        // Update Rendering System
        // child2.getTransform().lookAt(visual_system.getCamera().getTransform()->getPosition());
        // child2.getTransform().offsetRotation(Vector3::NegativeX(), 0.05f);
        visual_system.drawAsset(AssetSlot::Terrain, child1.getLocalMatrix());
        visual_system.drawAsset(AssetSlot::Fox, child2.getLocalMatrix());
        visual_system.drawAsset(AssetSlot::Cube, child3.getLocalMatrix());

        visual_system.render();

        // Stall until enough time has elapsed for 60 frames / second
        while (framerate_watch.Duration() < 1 / 60.f) {
        }
    }

    // Finish
    return 0;
}

// Defines the behavior of the window (appearance, user interaction, etc)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
    switch (uMsg) {
    // Destroys the Application on any Force Quit or Escape
    case WM_DESTROY: {
        ClipCursor(NULL);
        PostQuitMessage(0);
    }
        return 0;

    // Key Down
    case WM_KEYDOWN: {
        // Escape will always quit the application, just in case
        if (wParam == VK_ESCAPE) {
            ClipCursor(NULL);
            PostQuitMessage(0);
            return 0;
        }
        input_system.logWin32Input(uMsg, wParam);
    } break;

    // Key Up
    case WM_KEYUP:
        input_system.logWin32Input(uMsg, wParam);
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
