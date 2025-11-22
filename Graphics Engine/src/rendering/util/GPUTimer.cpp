#include "GPUTimer.h"

// imgui Includes
#include "rendering/ImGui.h"

namespace Engine {
namespace Graphics {

struct GPUTimerBatch {
  public:
    ID3D11Query* query_begin[2];
    ID3D11Query* query_end[2];
    bool used;

    GPUTimerBatch(ID3D11Device* device) {
        used = false;

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

IGPUTimer::IGPUTimer(GPUTimerBatch* _batch, bool _flag,
                     ID3D11DeviceContext* _context) {
    timer_batch = _batch;
    context = _context;
    flag = _flag;

    context->End(timer_batch->query_begin[flag]);
}
IGPUTimer::~IGPUTimer() {
    context->End(timer_batch->query_end[flag]);
    timer_batch->used = true;
}

ID3D11Device* GPUTimer::device = nullptr;
ID3D11DeviceContext* GPUTimer::context = nullptr;
bool GPUTimer::flag = 0;
ID3D11Query* GPUTimer::disjoint_queries[2] = {};
std::map<std::string, GPUTimerBatch*> GPUTimer::gpu_timers =
    std::map<std::string, GPUTimerBatch*>();
std::vector<std::string> GPUTimer::active_batches = std::vector<std::string>();

// Initialize:
// Initialize the GPUTimer by creating disjoint queries that can be used
// to get information about the GPU
void GPUTimer::Initialize(ID3D11Device* _device,
                          ID3D11DeviceContext* _context) {
    device = _device;
    context = _context;

    // Create two disjoint queries, for tracking GPU clock speed
    D3D11_QUERY_DESC query_desc = {};
    query_desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

    device->CreateQuery(&query_desc, &disjoint_queries[0]);
    device->CreateQuery(&query_desc, &disjoint_queries[1]);
}

// Begin, End:
// Begin and end timers within a frame.
void GPUTimer::BeginFrame() { context->Begin(disjoint_queries[flag]); }
IGPUTimer GPUTimer::TrackGPUTime(const std::string& name) {
    GPUTimerBatch* batch = nullptr;
    if (!gpu_timers.contains(name)) {
        batch = new GPUTimerBatch(device);
        gpu_timers[name] = batch;
    } else
        batch = gpu_timers[name];

    active_batches.push_back(name);
    return IGPUTimer(batch, flag, context);
}
void GPUTimer::EndFrame() { context->End(disjoint_queries[flag]); }

// DisplayTimes:
// Calculate and display the times of the PREVIOUS frame's timers
// to the ImGui menu.
void GPUTimer::DisplayGPUTimes() {
    flag = !flag;

    // Get clock speed information
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
    context->GetData(disjoint_queries[flag], &tsDisjoint, sizeof(tsDisjoint),
                     0);

    // For each timer, get its frame time and display it
    UINT64 begin, end;
    float frame_time;

    for (const std::string& name : active_batches) {
        GPUTimerBatch* timer_data = gpu_timers[name];

        // Get my query information
        context->GetData(timer_data->query_begin[flag], &begin,
                            sizeof(begin), 0);
        context->GetData(timer_data->query_end[flag], &end, sizeof(end), 0);

        // Calculate frame time
        frame_time =
            float(end - begin) / float(tsDisjoint.Frequency) * 1000.0f;

        // Display to ImGui
#if defined(_DEBUG)
        ImGui::Text("(GPU) %s: %f ms", name.c_str(), frame_time);
#endif
    }

    active_batches.clear();
}

} // namespace Graphics
} // namespace Engine