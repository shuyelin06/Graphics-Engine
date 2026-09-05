#pragma once

#include <functional>
#include <string>

#define IMGUI_ENABLED

namespace ImGuiHelper
{
void RegisterImGuiCallback(const std::string& path,
                           std::function<void(void)> callback);
void RenderImGui();
} // namespace ImGuiHelper

// Includes the libraries necessary for using ImGui in
// any part of the application.
#if defined(IMGUI_ENABLED)

#include "imgui/imgui.h"

// Converts the Vector3 address into a type ImGui accepts
#define Vec3ImGuiAddr(vec3) static_cast<float*>(&vec3.x)

#endif