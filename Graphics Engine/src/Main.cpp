
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
#include "physics/PhysicsSystem.h"
#include "rendering/VisualSystem.h"

#include "input/components/MovementHandler.h"

#include "rendering/VisualDebug.h"

#include "math/Compute.h"
#include "utility/Stopwatch.h"

// --- TEST
#include "datamodel/bvh/BVH.h"
// ---

using namespace Engine;
using namespace Engine::Physics;
using namespace Engine::Datamodel;
using namespace Engine::Input;
using namespace Engine::Graphics;

// Handles windows messages, including input.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Static reference to the input system for use in the window message callback
static InputSystem input_system;

// Main Function
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR pCmdLine, int nCmdShow) {
    Vector3 result = SphericalToEuler(sqrtf(26), 1.3734f, 5.3559f);
    Vector3 original = EulerToSpherical(result.x, result.y, result.z);

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
    VisualSystem visual_system = VisualSystem();
    visual_system.initialize(hwnd);

    // Create Physics System
    PhysicsSystem physics_system = PhysicsSystem();
    physics_system.initialize();

    // Create SceneGraph
    Scene scene_graph = Scene();
    Object& parent = scene_graph.createObject();

    Object& camera_obj = parent.createChild();
    visual_system.bindCameraComponent(&camera_obj);
    scene_graph.updateTerrainChunks(camera_obj.getTransform().getPosition());

    visual_system.bindTerrain(scene_graph.getTerrain());

    // Input Handling
    // MovementHandler movementHandler(&camera_obj.getTransform());
    physics_system.bindPhysicsObject(&camera_obj);

    //// Create Object Hierarchy
    // visual_system.bindAssetComponent(&parent_object, "Fox");
    // physics_system.bindPhysicsObject(&parent_object);

    // --- TESTING ENVIRONMENT
    BVH bvh = BVH();
    for (int i = 0; i < 3; i++) {
        const Vector3 v0 = Vector3(Random(-10.f, 10.f), Random(-10.f, 10.f),
                                   Random(-10.f, 10.f));
        const Vector3 v1 = Vector3(Random(-10.f, 10.f), Random(-10.f, 10.f),
                                   Random(-10.f, 10.f));
        const Vector3 v2 = Vector3(Random(-10.f, 10.f), Random(-10.f, 10.f),
                                   Random(-10.f, 10.f));

        bvh.addBVHTriangle(Triangle(v0, v1, v2), nullptr);
    }
    bvh.build();

    Vector3 ray_origin;
    Vector3 ray_direction = Vector3(0, -1, 0);
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

        /*static float o_arr[3] = {0.f, 0.f, 0.f};
        static float d_arr[3] = {0.f, -1.f, 0.f};
        ImGui::SliderFloat3("Origin", o_arr, -25.f, 25.f);
        ImGui::SliderFloat3("Direction", d_arr, -1.f, 1.f);

        ray_origin = Vector3(o_arr[0], o_arr[1], o_arr[2]);
        ray_direction = Vector3(d_arr[0], d_arr[1], d_arr[2]);
        ray_direction = ray_direction.unit();

        if (ImGui::Button("New BVH")) {
            std::vector<Triangle> triangles;
            for (int i = 0; i < 55; i++) {
                const float extent = 5.f;
                const Vector3 center =
                    Vector3(Random(-50.f, 50.f), Random(-50.f, 50.f),
                            Random(-50.f, 50.f));
                const Vector3 v0 = center + Vector3(Random(-extent, extent),
                                                    Random(-extent, extent),
                                                    Random(-extent, extent));
                const Vector3 v1 = center + Vector3(Random(-extent, extent),
                                                    Random(-extent, extent),
                                                    Random(-extent, extent));
                const Vector3 v2 = center + Vector3(Random(-extent, extent),
                                                    Random(-extent, extent),
                                                    Random(-extent, extent));

                bvh.addBVHTriangle(Triangle(v0, v1, v2), nullptr);
            }
            bvh.build();
        }

        BVHRayCast cast = bvh.raycast(ray_origin, ray_direction);
        if (cast.hit) {
            VisualDebug::DrawLine(ray_origin,
                                  ray_origin + ray_direction * cast.t,
                                  Color::Green());
            VisualDebug::DrawPoint(ray_origin + ray_direction * cast.t, 2.5f,
                                   Color ::Green());
        } else {
            VisualDebug::DrawLine(
                ray_origin, ray_origin + ray_direction * 100.f, Color::Red());
        }

        bvh.debugDrawBVH();*/

        const TLAS& tlas = scene_graph.getTerrain()->getTLAS();
        BVHRayCast cast = tlas.raycast(camera_obj.getTransform().getPosition(),
                                       camera_obj.getTransform().forward());
        if (cast.hit) {
            const Triangle& tri = cast.hit_triangle->triangle;
            VisualDebug::DrawLine(tri.vertex(0), tri.vertex(1), Color::Green());
            VisualDebug::DrawLine(tri.vertex(1), tri.vertex(2), Color::Green());
            VisualDebug::DrawLine(tri.vertex(2), tri.vertex(0), Color::Green());
        }

        // Dispatch Input Data
        // movementHandler.update();
        input_system.update();

        // Update Physics System
        physics_system.update();

        // Update Datamodel
        scene_graph.updateObjects();
        static int update_time = 0;
        if (update_time++ > 10) {
            scene_graph.updateTerrainChunks(
                camera_obj.getTransform().getPosition());
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
    if (input_system.dispatchWin32Input(hwnd, uMsg, wParam, lParam))
        return true;

    // Default Window Behavior
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
