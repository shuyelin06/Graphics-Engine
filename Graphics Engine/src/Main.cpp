
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

#include <thread>

#include "datamodel/SceneGraph.h"
#include "input/InputSystem.h"
#include "physics/PhysicsSystem.h"
#include "rendering/VisualSystem.h"

#include "rendering/VisualDebug.h"
#include "utility/Stopwatch.h"

// --- TEST

// ---

using namespace Engine;
using namespace Engine::Physics;
using namespace Engine::Datamodel;
using namespace Engine::Input;
using namespace Engine::Graphics;

// Handles windows messages, including input.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Static reference to the input system for use in the window message callback
static InputSystem* input_system_handle;

// Main Function
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR pCmdLine, int nCmdShow) {
    // Create a Window Class with the OS
    const wchar_t CLASS_NAME[] = L"Main";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;   // Function pointer to WindowProc
    wc.hInstance = hInstance;      // Handle to this application instance
    wc.lpszClassName = CLASS_NAME; // String identifying the window class

    RegisterClass(&wc);

    // Create a window instance from this class. HWND will contain a handle
    // to the window created.
    HWND hwnd = CreateWindowEx(0,                  // Optional window styles.
                               CLASS_NAME,         // Window class
                               L"Graphics Engine", // Window text
                               WS_BORDER,          // Window style

                               // Size and position
                               CW_USEDEFAULT, CW_USEDEFAULT, 960, 640,

                               NULL,      // Parent window
                               NULL,      // Menu
                               hInstance, // Instance handle
                               NULL       // Additional application data
    );

    assert(hwnd != NULL); // Check Success

    ShowWindow(hwnd, nCmdShow); // Set Window Visible

    // Seed Random Number Generator
    srand(0);

    // --- Create my Systems ---
    InputSystem input_system = InputSystem(hwnd);
    input_system_handle = &input_system;
    VisualSystem visual_system = VisualSystem(hwnd);
    PhysicsSystem physics_system = PhysicsSystem();

    // --- Create my Scene ---
    Scene scene_graph = Scene();
    scene_graph.updateTerrainChunks(0.f, 0.f, 0.f);

    Object& parent = scene_graph.createObject();

    // Bind a Camera
    Object& camera_obj = parent.createChild();
    visual_system.bindCameraComponent(&camera_obj);

    // Bind Terrain
    visual_system.bindTerrain(scene_graph.getTerrain());

    // Bind Movement Physics
    physics_system.bindPhysicsObject(&camera_obj);

    //// Create Object Hierarchy
    // visual_system.bindAssetComponent(&parent_object, "Fox");
    // physics_system.bindPhysicsObject(&parent_object);

    // --- TESTING ENVIRONMENT
    std::thread thread;
    // ---

    // Begin window messaging loop
    MSG msg = {};
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

        // Dispatch Input Data
        input_system.update();

        const TLAS& tlas = scene_graph.getTerrain()->getTLAS();
        BVHRayCast cast = tlas.raycast(camera_obj.getTransform().getPosition(),
                                       camera_obj.getTransform().forward());
        if (cast.hit) {
            const Triangle& tri = cast.hit_triangle->triangle;
            VisualDebug::DrawLine(tri.vertex(0), tri.vertex(1), Color::Green());
            VisualDebug::DrawLine(tri.vertex(1), tri.vertex(2), Color::Green());
            VisualDebug::DrawLine(tri.vertex(2), tri.vertex(0), Color::Green());
        }

        // Pull Data for Rendering
        visual_system.pullDatamodelData();

        // Update Physics System
        physics_system.update();

        // Update Datamodel
        scene_graph.updateObjects();
        static int update_time = 0;
        if (update_time++ > 10) {
            const Vector3 pos = camera_obj.getTransform().getPosition();
            /*thread = std::thread(&Scene::updateTerrainChunks, scene_graph,
                                 pos.x, pos.y, pos.z);*/
            scene_graph.updateTerrainChunks(pos.x, pos.y, pos.z);

            update_time = 0;
        }

        // Render Objects
        visual_system.render();

        // Stall until enough time has elapsed for 60 frames / second
        while (framerate_watch.Duration() < 1 / 60.f) {
        }
    }

    // Shutdown all systems
    visual_system.shutdown();

    // Finish
    return 0;
}

// Defines the behavior of the window (appearance, user interaction, etc)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
    // Escape will always quit the application, just in case
    if (uMsg == WM_DESTROY || (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE)) {
        ClipCursor(NULL);
        PostQuitMessage(0);
        return 0;
    }

#if defined(_DEBUG)
    // ImGui Input
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
        HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;
#endif

    // Input System Input
    if (input_system_handle->dispatchWin32Input(hwnd, uMsg, wParam, lParam))
        return true;

    // Default Window Behavior
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
