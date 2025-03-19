#include "GPUTimer.h"

// imgui Includes
#include "rendering/ImGui.h"

namespace Engine {
namespace Graphics {

struct GPUTimerBatch {
  public:
    ID3D11Query* query_begin[2];
    ID3D11Query* query_end[2];

    GPUTimerBatch(ID3D11Device* device) {
        // Create two queries, one for the beginning of our tracking
        // and one for the end. Internally, we double buffer
        // to avoid synchronization issues.
        D3D11_QUERY_DESC query_desc = {};
        query_desc.Query = D3D11_QUERY_TIMESTAMP;

        device->CreateQuery(&query_desc, &query_begin[0]);
        device->CreateQuery(&query_desc, &query_begin[1]);
        device->CreateQuery(&query_desc, &query_end[0]);
        device->CreateQuery(&query_desc, &query_end[1]);
    }
};

GPUTimer::GPUTimer() : gpu_timers() {
    device = nullptr;
    context = nullptr;

    flag = 0;

    disjoint_queries[0] = nullptr;
    disjoint_queries[1] = nullptr;
}

// Initialize:
// Initialize the GPUTimer by creating disjoint queries that can be used
// to get information about the GPU
void GPUTimer::initialize(ID3D11Device* _device,
                          ID3D11DeviceContext* _context) {
    device = _device;
    context = _context;

    // Create two disjoint queries, for tracking GPU clock speed
    D3D11_QUERY_DESC query_desc = {};
    query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

    device->CreateQuery(&query_desc, &disjoint_queries[0]);
    device->CreateQuery(&query_desc, &disjoint_queries[1]);
}

// CreateTimer:
// Creates a timer that can track a specific GPU stage, with a specific name.
void GPUTimer::createTimer(std::string name) {
    if (!gpu_timers.contains(name)) {
        gpu_timers[name] = new GPUTimerBatch(device);
    }
}

// Begin, End:
// Begin and end timers within a frame.
void GPUTimer::beginFrame() { context->Begin(disjoint_queries[flag]); }
void GPUTimer::beginTimer(std::string name) {
    if (gpu_timers.contains(name))
        context->End(gpu_timers[name]->query_begin[flag]);
}
void GPUTimer::endTimer(std::string name) {
    if (gpu_timers.contains(name))
        context->End(gpu_timers[name]->query_end[flag]);
}
void GPUTimer::endFrame() { context->End(disjoint_queries[flag]); }

// DisplayTimes:
// Calculate and display the times of the PREVIOUS frame's timers
// to the ImGui menu.
void GPUTimer::displayTimes() {
    flag = !flag;

    // Get clock speed information
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
    context->GetData(disjoint_queries[flag], &tsDisjoint, sizeof(tsDisjoint),
                     0);

    // For each timer, get its frame time and display it
    UINT64 begin, end;
    float frame_time;

    for (const auto& pair : gpu_timers) {
        std::string name = pair.first;
        GPUTimerBatch* timer_data = pair.second;

        // Get my query information
        context->GetData(timer_data->query_begin[flag], &begin, sizeof(begin),
                         0);
        context->GetData(timer_data->query_end[flag], &end, sizeof(end), 0);

        // Calculate frame time
        frame_time = float(end - begin) / float(tsDisjoint.Frequency) * 1000.0f;

        // Display to ImGui
        ImGui::Text("(GPU) %s: %f ms", name.c_str(), frame_time);
    }
}

} // namespace Graphics
} // namespace Engine