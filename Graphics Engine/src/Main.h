#pragma once

#include "rendering/VisualEngine.h" // Graphics Engine
#include "input/InputEngine.h"		// Input Engine

using namespace Engine;

// Handles all input
extern Input::InputEngine input_engine;

// Handles all rendering
extern Graphics::VisualEngine graphics_engine;