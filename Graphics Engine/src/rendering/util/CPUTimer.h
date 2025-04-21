#pragma once

#include <map>
#include <mutex>
#include <string>

#include "utility/Stopwatch.h"

namespace Engine {
namespace Graphics {

struct CPUTimerBatch;

// Class CPUTimer:
// Can be used to track the amount of time a batch of CPU
// commands take.
// Initialization is handled by the visual system
// (as the data is displayed on the ImGui menu).
class CPUTimer {
  private:
    static CPUTimer* system_timer;
    static std::mutex mutex;

    std::map<std::string, CPUTimerBatch*> cpu_timers;

    CPUTimer();

  public:
    static void Initialize();

    static void CreateCPUTimer(const std::string& name);
    static void BeginCPUTimer(const std::string& name);
    static void EndCPUTimer(const std::string& name);

    // Display Current Frame's Times to ImGui
    static void DisplayCPUTimes();
};

} // namespace Graphics
} // namespace Engine