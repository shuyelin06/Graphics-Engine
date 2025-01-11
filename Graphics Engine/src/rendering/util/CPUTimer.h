#pragma once

#include <map>
#include <string>

#include "utility/Stopwatch.h"

namespace Engine {
namespace Graphics {

struct CPUTimerBatch;

// Class CPUTimer:
// Can be used to track the amount of time a batch of CPU
// commands take
class CPUTimer {
  private:
    std::map<std::string, CPUTimerBatch*> cpu_timers;

  public:
    CPUTimer();

    void initialize();

    void createTimer(std::string name);

    void beginTimer(std::string name);
    void endTimer(std::string name);

    // Display Current Frame's Times to ImGui
    void displayTimes();
};

} // namespace Graphics
} // namespace Engine