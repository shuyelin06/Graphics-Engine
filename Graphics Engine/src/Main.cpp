
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

// ----- TESTING -----
#include "datamodel/TreeGenerator.h"
#include "physics/collisions/AABBTree.h"
// -----

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
    VisualSystem visual_system = VisualSystem(hwnd);
    visual_system.initialize();

    // Create Physics System
    PhysicsSystem physics_system = PhysicsSystem();
    physics_system.initialize();

    // Create SceneGraph
    Scene scene_graph = Scene();

    // Bind Camera
    MovementHandler movementHandler(visual_system.getCamera().getTransform());
    visual_system.getCamera().getTransform()->setPosition(0, 50.f, 0);

    // Create Object Hierarchy
    Object& parent_object = scene_graph.createObject();

    Object& sun_light = parent_object.createChild();
    visual_system.bindShadowLightObject(&sun_light);

    /* Object& child2 = parent_object.createChild();
     AssetObject* asset2 = visual_system.bindAssetObject(&child2, "Capybara");
     child2.getTransform().offsetRotation(Vector3::PositiveY(), PI);
     child2.getTransform().setScale(5, 5, 5);
     child2.getTransform().setPosition(Random(-2.5f, 2.5f), Random(-2.5f, 2.5f),
                                       Random(15, 25));*/

    /*std::vector<Vector3> points;
    points.push_back(Vector3(-2.5, -2.5, -2.5));
    points.push_back(Vector3(-2.5, -2.5, 2.5));
    points.push_back(Vector3(-2.5, 2.5, -2.5));
    points.push_back(Vector3(-2.5, 2.5, 2.5));
    points.push_back(Vector3(2.5, -2.5, -2.5));
    points.push_back(Vector3(2.5, -2.5, 2.5));
    points.push_back(Vector3(2.5, 2.5, -2.5));
    points.push_back(Vector3(2.5, 2.5, 2.5));
    physics_system.addCollisionHull("Box", points);
    PhysicsObject* p1 = physics_system.bindPhysicsObject(&child2);
    CollisionObject* c1 = physics_system.bindCollisionObject(p1, "Box");*/

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
        movementHandler.update();
        input_system.update();

        // Update Physics System
        physics_system.update();

        // Update terrain (manually)
        if (true) {
            const Vector3& cam_pos =
                visual_system.getCamera().getTransform()->getPosition();
            scene_graph.updateSceneCenter(cam_pos.x, cam_pos.z);

            for (int i = 0; i < TERRAIN_NUM_CHUNKS; i++) {
                for (int j = 0; j < TERRAIN_NUM_CHUNKS; j++) {
                    TerrainChunk* chunk = scene_graph.getTerrainChunk(i,j);

                    if (!chunk->hasVisualTerrain()) {
                        visual_system.bindVisualTerrain(chunk);
                    }
                }
            }
        }

        //// TODO: THIS CODE IS WRONG
        // child2.getTransform().lookAt(
        //     visual_system.getCamera().getTransform()->getPosition());
        

        // child2.getTransform().offsetRotation(Vector3::PositiveY(), PI / 20);
        sun_light.getTransform().setViewDirection(Vector3(0, -0.25f, 0.75f));
        Vector3 position =
            visual_system.getCamera().getTransform()->getPosition() +
            sun_light.getTransform().backward() * 75; // 75 OG
        sun_light.getTransform().setPosition(position.x, position.y,
                                             position.z);

        // Submit Object Render Requests
        scene_graph.updateAndRenderObjects();
        scene_graph.updateAndRenderTerrain();

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
