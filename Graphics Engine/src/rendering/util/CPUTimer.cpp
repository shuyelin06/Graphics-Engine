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

CPUTimer* CPUTimer::system_timer = nullptr;
std::mutex CPUTimer::mutex = std::mutex();
void CPUTimer::Initialize() { system_timer = new CPUTimer(); }

// CreateTimer:
// Creates a new timer associated with some name
void CPUTimer::CreateCPUTimer(const std::string& name) {
    std::unique_lock<std::mutex> lock(mutex);
    if (!system_timer->cpu_timers.contains(name))
        system_timer->cpu_timers[name] = new CPUTimerBatch();
}

// Begin, End:
// Begin and end a CPU timer
void CPUTimer::BeginCPUTimer(const std::string& name) {
    std::unique_lock<std::mutex> lock(mutex);
    if (system_timer->cpu_timers.contains(name))
        system_timer->cpu_timers[name]->timer.Reset();
}

void CPUTimer::EndCPUTimer(const std::string& name) {
    std::unique_lock<std::mutex> lock(mutex);
    if (system_timer->cpu_timers.contains(name))
        system_timer->cpu_timers[name]->duration =
            (float)system_timer->cpu_timers[name]->timer.Duration();
}

// DisplayTimes:
// Calculate and display the times of the PREVIOUS frame's timers
// to the ImGui menu.
void CPUTimer::DisplayCPUTimes() {
    std::unique_lock<std::mutex> lock(mutex);
    for (const auto& pair : system_timer->cpu_timers) {
        const std::string& name = pair.first;
        const float duration = pair.second->duration;

#if defined(_DEBUG)
        ImGui::Text("(CPU) %s: %f ms", name.c_str(), duration);
#endif
    }
}

} // namespace Graphics
} // namespace Engine