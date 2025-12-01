#pragma once

#define IMGUI_ENABLED

// Includes the libraries necessary for using ImGui in
// any part of the application.
#if defined(IMGUI_ENABLED)

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

// Converts the Vector3 address into a type ImGui accepts
#define Vec3ImGuiAddr(vec3) static_cast<float*>(&vec3.x)

#endif