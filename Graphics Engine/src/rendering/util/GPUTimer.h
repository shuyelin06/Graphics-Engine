#pragma once

#include <map>
#include <string>
#include <vector>

#include "rendering/Direct3D11.h"

namespace Engine {
namespace Graphics {

struct GPUTimerBatch;

// IGPUTimer Class:
// GPUTimer interface that begins and ends queries with the constructor and
// destructor (respectively). Easier to use than manually calling begin and end
// frame.
class IGPUTimer {
  private:
    GPUTimerBatch* timer_batch;
    bool flag;

    ID3D11DeviceContext* context;

  public:
    IGPUTimer(GPUTimerBatch* batch, bool flag, ID3D11DeviceContext* context);
    ~IGPUTimer();
};

// Class GPUTimer:
// Can be used to track the amount of time batches of GPU commands take.
class GPUTimer {
  private:
    static ID3D11Device* device;
    static ID3D11DeviceContext* context;

    static bool flag;

    static ID3D11Query* disjoint_queries[2];
    static std::map<std::string, GPUTimerBatch*> gpu_timers;
    static std::vector<std::string> active_batches;

  public:
    static void Initialize(ID3D11Device* _device,
                           ID3D11DeviceContext* _context);

    // Call before and after any render frame
    static void BeginFrame();
    static void EndFrame();

    // Use a timer to begin tracking the GPU time
    static IGPUTimer TrackGPUTime(const std::string& name);

    // Display Previous Frame's Times to ImGui
    static void DisplayGPUTimes();
};
} // namespace Graphics
} // namespace Engine