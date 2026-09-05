#pragma once

// Only use UTILITY_IMGUI_ENABLED within the cpp files.
#if defined(ENABLE_GLOBAL_IMGUI)
#define UTILITY_IMGUI_ENABLED 1
#include "external/imgui.h"
#endif