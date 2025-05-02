#pragma once

#include <map>
#include <mutex>
#include <string>

#include "utility/Stopwatch.h"

namespace Engine {
namespace Graphics {

struct CPUTimerBatch;

// ICPUTimer Class:
// CPUTimer interface that begins and ends queries with the constructor and
// destructor (respectively). Easier to use than manually calling begin and end
// frame.
// MAKE SURE THIS IS ALLOCATED ON THE STACK. Assign the output of
// TrackCPUTime() to a local variable, and keep it until the end of the scope.
class ICPUTimer {
  private:
    CPUTimerBatch* timer_batch;

  public:
    ICPUTimer(CPUTimerBatch* batch);
    ~ICPUTimer();
};

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
    static ICPUTimer TrackCPUTime(const std::string& name);

    // Display Current Frame's Times to ImGui
    static void DisplayCPUTimes();
};

} // namespace Graphics
} // namespace Engine