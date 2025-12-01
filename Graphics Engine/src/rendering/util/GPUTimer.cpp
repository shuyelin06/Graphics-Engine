#include "GPUTimer.h"

// imgui Includes
#include "rendering/ImGui.h"

namespace Engine {
namespace Graphics {

struct GPUTimerBatch {
  public:
    struct QueryGroup {
        ID3D11Query* begin;
        ID3D11Query* end;
    };
    QueryGroup queries[NUM_QUERY_GROUPS];
    bool used;

    GPUTimerBatch(ID3D11Device* device) {
        used = false;

        // Create two queries, one for the beginning of our tracking
        // and one for the end. Internally, we double buffer
        // to avoid synchronization issues.
        D3D11_QUERY_DESC query_desc = {};
        query_desc.Query = D3D11_QUERY_TIMESTAMP;

        for (int i = 0; i < NUM_QUERY_GROUPS; i++) {
            device->CreateQuery(&query_desc, &queries[i].begin);
            device->CreateQuery(&query_desc, &queries[i].end);
        }
    }
};

IGPUTimer::IGPUTimer(GPUTimerBatch* _batch, uint64_t _frame,
                     ID3D11DeviceContext* _context) {
    timer_batch = _batch;
    context = _context;
    frame = _frame;

    context->End(timer_batch->queries[frame % NUM_QUERY_GROUPS].begin);
}
IGPUTimer::~IGPUTimer() {
    context->End(timer_batch->queries[frame % NUM_QUERY_GROUPS].end);
    timer_batch->used = true;
}

ID3D11Device* GPUTimer::device = nullptr;
ID3D11DeviceContext* GPUTimer::context = nullptr;
ID3D11Query* GPUTimer::disjoint_queries[NUM_QUERY_GROUPS] = {};
uint64_t GPUTimer::frame = 0;
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

    for (int i = 0; i < NUM_QUERY_GROUPS; i++) {
        device->CreateQuery(&query_desc, &disjoint_queries[i]);
    }
}

// Begin, End:
// Begin and end timers within a frame.
void GPUTimer::BeginFrame(const uint64_t _frame) {
    frame = _frame;
    context->Begin(disjoint_queries[frame % NUM_QUERY_GROUPS]);
}
IGPUTimer GPUTimer::TrackGPUTime(const std::string& name) {
    GPUTimerBatch* batch = nullptr;
    if (!gpu_timers.contains(name)) {
        batch = new GPUTimerBatch(device);
        gpu_timers[name] = batch;
    } else
        batch = gpu_timers[name];

    active_batches.push_back(name);
    return IGPUTimer(batch, frame, context);
}
void GPUTimer::EndFrame() {
    context->End(disjoint_queries[frame % NUM_QUERY_GROUPS]);
}

// DisplayTimes:
// Calculate and display the times of the PREVIOUS frame's timers
// to the ImGui menu.
void GPUTimer::DisplayGPUTimes() {
    int target_query = (frame + 1) % NUM_QUERY_GROUPS;

    // Get clock speed information
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
    const auto result = context->GetData(disjoint_queries[target_query], &tsDisjoint,
                     sizeof(tsDisjoint), 0);
    if (!SUCCEEDED(result))
        return;

    // For each timer, get its frame time and display it
    UINT64 begin, end;
    float frame_time;

    for (const std::string& name : active_batches) {
        GPUTimerBatch* timer_data = gpu_timers[name];

        // Get my query information
        assert(context->GetData(timer_data->queries[target_query].begin, &begin,
                                sizeof(begin), 0) != S_FALSE);
        assert(context->GetData(timer_data->queries[target_query].end, &end,
                                sizeof(end), 0) != S_FALSE);

        // Calculate frame time
        frame_time = float(end - begin) * 1000.0f / float(tsDisjoint.Frequency);

        // Display to ImGui
#if defined(_DEBUG)
        ImGui::Text("(GPU) %s: %f ms", name.c_str(), frame_time);
#endif
    }

    active_batches.clear();
}

} // namespace Graphics
} // namespace Engine