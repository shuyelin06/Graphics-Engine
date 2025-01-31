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

#include "rendering/ResourceManager.h"

#include "math/Compute.h"
#include "utility/Stopwatch.h"

// TEMP
#include "math/geometry/ConvexHull.h"

using namespace Engine;
using namespace Engine::Simulation;
using namespace Engine::Datamodel;
using namespace Engine::Input;
using namespace Engine::Graphics;

// Handles windows messages, including input.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Static reference to the input system for use in the window message callback
static InputSystem input_system;

#include "math/geometry/AABB.h"
#include "math/geometry/GJK.h"
#include "math/geometry/GJKSupport.h"

// Main Function
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR pCmdLine, int nCmdShow) {
    Vector3 result = Compute::SphericalToEuler(sqrtf(26), 1.3734f, 5.3559f);
    Vector3 original = Compute::EulerToSpherical(result.x, result.y, result.z);

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

    // Create Input System
    input_system = InputSystem();
    input_system.initialize(hwnd);

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
    visual_system.getCamera().getTransform()->setPosition(0, 10, 0);

    // TEST: GJK
    GJKSupportPointSet* supp1 = new GJKSupportPointSet();
    GJKSupportPointSet* supp2 = new GJKSupportPointSet();
    GJKSolver solver = GJKSolver(supp1, supp2);

    // Create Object Hierarchy
    Object& parent_object = scene_graph.createObject();

    Object& child1 = parent_object.createChild();

    Object& child2 = parent_object.createChild();
    child2.setAsset(AssetSlot::Fox);
    child2.getTransform().setScale(5, 5, 5);
    child2.getTransform().setPosition(Compute::Random(-2.5f, 2.5f),
                                      Compute::Random(-2.5f, 2.5f),
                                      Compute::Random(15, 25));

    // Begin window messaging loop
    MSG msg = {};
    bool close = false;

    Utility::Stopwatch framerate_watch = Utility::Stopwatch();

    VisualDebug::DrawPoint(Vector3(0, 0, 0), 1, Color::White());
    VisualDebug::DrawPoint(Vector3(1, 0, 0), 1, Color::Red());
    VisualDebug::DrawPoint(Vector3(0, 1, 0), 1, Color::Green());
    VisualDebug::DrawPoint(Vector3(0, 0, 1), 1, Color::Blue());

    VisualDebug::DrawLine(Vector3(0, 0, 0), Vector3(5, 0, 0),
                          Color::Red());                      // X Red
    VisualDebug::DrawLine(Vector3(0, 0, 0), Vector3(0, 5, 0), // Y Green
                          Color::Green());
    VisualDebug::DrawLine(Vector3(0, 0, 0), Vector3(0, 0, 5), // Z Blue
                          Color::Blue());

    std::vector<Vector3> point_cloud; 
    
    for (int i = 0; i < 20; i++) {
        point_cloud.push_back(Vector3(Compute::Random(-10.f, 10.f), Compute::Random(-10.f, 10.f), Compute::Random(-10.f, 10.f)));
    }

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
        movementHandler.update();
        input_system.update();

        // Update Physics System
        physics_system.update();

        // TODO: THIS CODE IS WRONG
        child2.getTransform().lookAt(
            visual_system.getCamera().getTransform()->getPosition());

        // TESTING
        static float pos[3] = {};
        ImGui::SliderFloat3("Position:", pos, -20.f, 20.f);

        point_cloud[0].set(Vector3(pos[0], pos[1], pos[2]));

        for (const Vector3& point : point_cloud) {
            VisualDebug::DrawPoint(point, 0.5f);
        }

        ConvexHull* convex_hull = ConvexHull::QuickHull(point_cloud);

        convex_hull->debugDrawConvexHull();

        // Submit Object Render Requests
        std::vector<AssetRenderRequest> asset_requests;
        scene_graph.updateAndRenderObjects(asset_requests);
        for (const AssetRenderRequest& request : asset_requests)
            visual_system.drawAsset(request);

        // Submit Terrain Render Requests
        std::vector<TerrainRenderRequest> terrain_requests;
        scene_graph.updateAndRenderTerrain(terrain_requests);
        for (const TerrainRenderRequest& request : terrain_requests)
            visual_system.drawTerrain(request);

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
    if (input_system.dispatchWin32Input(hwnd, uMsg, wParam, lParam))
        return true;

    // Default Window Behavior
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
