#pragma once

#include <string>
#include <map>

#include "rendering/Direct3D11.h"

namespace Engine {
namespace Graphics {

struct GPUTimerBatch;

// Class GPUTimer:
// Can be used to track the amount of time batches of GPU commands take.
class GPUTimer {
  private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    bool flag;

    ID3D11Query* disjoint_queries[2];
    std::map<std::string, GPUTimerBatch*> gpu_timers;

  public:
    GPUTimer();

    void initialize(ID3D11Device* _device, ID3D11DeviceContext* _context);

    void createTimer(std::string name);

    void beginFrame();
    void beginTimer(std::string name);
    void endTimer(std::string name);
    void endFrame();

    // Display Previous Frame's Times to ImGui
    void displayTimes(); 

};
} // namespace Graphics
} // namespace Engine