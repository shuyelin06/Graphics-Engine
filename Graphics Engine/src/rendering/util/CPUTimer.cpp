#include "CPUTimer.h"

// imgui Includes
#include "rendering/ImGui.h"

namespace Engine {
using namespace Utility;

namespace Graphics {
struct CPUTimerBatch {
    Stopwatch timer;
    float duration;
    bool used;

    CPUTimerBatch() = default;
};

ICPUTimer::ICPUTimer(CPUTimerBatch* batch) {
    timer_batch = batch;
    timer_batch->timer.Reset();
}
ICPUTimer::~ICPUTimer() {
    timer_batch->duration = (float)timer_batch->timer.Duration();
    timer_batch->used = true;
}

CPUTimer::CPUTimer() = default;

CPUTimer* CPUTimer::system_timer = nullptr;
std::mutex CPUTimer::mutex = std::mutex();
void CPUTimer::Initialize() { system_timer = new CPUTimer(); }

// Begin, End:
// Begin and end a CPU timer
ICPUTimer CPUTimer::TrackCPUTime(const std::string& name) {
    std::unique_lock<std::mutex> lock(mutex);
    if (!system_timer->cpu_timers.contains(name))
        system_timer->cpu_timers[name] = new CPUTimerBatch();
    return ICPUTimer(system_timer->cpu_timers[name]);
}

// DisplayTimes:
// Calculate and display the times of the PREVIOUS frame's timers
// to the ImGui menu.
void CPUTimer::DisplayCPUTimes() {
    std::unique_lock<std::mutex> lock(mutex);
    for (auto& pair : system_timer->cpu_timers) {
        const std::string& name = pair.first;
        const float duration = pair.second->duration;

        if (pair.second->used) {
            pair.second->used = false;
#if defined(_DEBUG)
            ImGui::Text("(CPU) %s: %f ms", name.c_str(), duration);
#endif
        }
    }
}

} // namespace Graphics
} // namespace Engine