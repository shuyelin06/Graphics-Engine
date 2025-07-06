#pragma once

// Includes the libraries necessary for using ImGui in
// any part of the application.
// Only included on debug build
#if defined(_DEBUG)

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#endif