#include "CPUTimer.h"

// imgui Includes
#include "rendering/ImGui.h"

namespace Engine {
using namespace Utility;

namespace Graphics {
struct CPUTimerBatch {
    Stopwatch timer;
    float duration;

    CPUTimerBatch() = default;
};

CPUTimer::CPUTimer() = default;

void CPUTimer::initialize() {}

// CreateTimer:
// Creates a new timer associated with some name
void CPUTimer::createTimer(std::string name) {
    if (!cpu_timers.contains(name))
        cpu_timers[name] = new CPUTimerBatch();
}

// Begin, End:
// Begin and end a CPU timer
void CPUTimer::beginTimer(std::string name) {
    if (cpu_timers.contains(name))
        cpu_timers[name]->timer.Reset();
}

void CPUTimer::endTimer(std::string name) {
    if (cpu_timers.contains(name)) 
        cpu_timers[name]->duration = (float) cpu_timers[name]->timer.Duration();
}

// DisplayTimes:
// Calculate and display the times of the PREVIOUS frame's timers
// to the ImGui menu.
void CPUTimer::displayTimes() {
    for (const auto& pair : cpu_timers) {
        std::string name = pair.first;
        float duration = pair.second->duration;

        ImGui::Text("(CPU) %s: %f ms", name.c_str(), duration);
    }
}

} // namespace Graphics
} // namespace Engine