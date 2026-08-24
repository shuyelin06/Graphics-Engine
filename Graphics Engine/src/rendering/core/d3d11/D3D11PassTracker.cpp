#include "D3D11PassTracker.h"

#include <assert.h>

namespace Engine
{
namespace Graphics
{
D3D11PassTracker::D3D11PassTracker(ID3D11Device* device)
{
    // Initialize each FrameQuery in my queries array.
    D3D11_QUERY_DESC queryDesc = {};
    queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

    HRESULT result;
    for (FrameQuery& frameQuery : queries)
    {
        result = device->CreateQuery(&queryDesc, &frameQuery.disjointQuery);
        assert(SUCCEEDED(result));
    }
}
D3D11PassTracker::~D3D11PassTracker()
{
    // Release each query in my frame queries array
    for (FrameQuery& frameQuery : queries)
    {
        if (frameQuery.disjointQuery)
            frameQuery.disjointQuery->Release();

        for (PassQuery& passQuery : frameQuery.passes)
        {
            if (passQuery.queryBegin)
                passQuery.queryBegin->Release();
            if (passQuery.queryEnd)
                passQuery.queryEnd->Release();
        }
    }
}

void D3D11PassTracker::beginFrame(uint64_t frameNumber,
                                  ID3D11DeviceContext* context)
{
    // Huh? We somehow went backwards in frames?
    assert(activeFrame == 0 || frameNumber > activeFrame);

    // Find the next FrameQuery we will use
    const size_t nextQuery = queriesHead % kNumActiveQueries;
    // Advance queries head
    queriesHead = nextQuery;
    queriesHead++;

    activeFrameQuery = &queries[nextQuery];

    // If used previously, harvest the information
    if (activeFrameQuery->frame != UINT64_MAX)
    {
        harvestFrameQuery(*activeFrameQuery, context);
    }

    // Clear and reuse the frame query
    activeFrame = frameNumber;
    activeFrameQuery->frame = activeFrame;
    activeFrameQuery->passesHead = 0;
    context->Begin(activeFrameQuery->disjointQuery);
}

void D3D11PassTracker::harvestFrameQuery(const FrameQuery& frameQuery,
                                         ID3D11DeviceContext* context)
{
    // Harvest the results of the query information if interpretable
    HRESULT result;
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
    result = context->GetData(frameQuery.disjointQuery, &tsDisjoint,
                              sizeof(tsDisjoint), 0);

    if (SUCCEEDED(result))
    {
        UINT64 begin, end;
        float frameTime;

        for (int i = 0; i < frameQuery.passesHead; i++)
        {
            const PassQuery& passQuery = frameQuery.passes[i];

            result = context->GetData(passQuery.queryBegin, &begin,
                                      sizeof(begin), 0);
            if (!SUCCEEDED(result))
                continue;
            result = context->GetData(passQuery.queryEnd, &end, sizeof(end), 0);
            if (!SUCCEEDED(result))
                continue;

            frameTime =
                float(end - begin) * 1000.f / float(tsDisjoint.Frequency);

            // Save results, creating new entry into the pass info map if needed
            auto [iter, inserted] =
                passInfo.insert({passQuery.passName, PassInformation()});
            PassInformation& passInfo = iter->second;
            passInfo.lastFrameUsed = frameQuery.frame;
            passInfo.frameTime[iter->second.frameTimeHead] = frameTime;
            passInfo.frameTimeHead =
                (passInfo.frameTimeHead + 1) % kNumActiveQueries;
        }
    }
}

void D3D11PassTracker::endFrame(ID3D11DeviceContext* context)
{
    assert(activeFrameQuery);
    context->End(activeFrameQuery->disjointQuery);
    activeFrameQuery = nullptr;
}

void D3D11PassTracker::beginPass(const char* passName,
                                 ID3D11Device* device,
                                 ID3D11DeviceContext* context)
{
    // There is an ongoing pass! Call endPass before beginning another pass
    assert(activePassQuery == nullptr);
    // beginFrame not called before this pass!
    assert(activeFrameQuery);

    // Reuse exising query object
    if (activeFrameQuery->passesHead < activeFrameQuery->passes.size())
    {
        activePassQuery =
            &activeFrameQuery->passes[activeFrameQuery->passesHead++];
    }
    // Create new query object
    else
    {
        activePassQuery = &activeFrameQuery->passes.emplace_back();
        activeFrameQuery->passesHead++;

        HRESULT result;
        D3D11_QUERY_DESC desc = {};
        desc.Query = D3D11_QUERY_TIMESTAMP;

        result = device->CreateQuery(&desc, &activePassQuery->queryBegin);
        assert(SUCCEEDED(result));
        result = device->CreateQuery(&desc, &activePassQuery->queryEnd);
        assert(SUCCEEDED(result));
    }

    // Initialize query
    assert(activePassQuery);
    activePassQuery->passName = passName;
    context->End(activePassQuery->queryBegin);
}

void D3D11PassTracker::endPass(ID3D11DeviceContext* context)
{
    // No active pass!
    assert(activePassQuery);

    context->End(activePassQuery->queryEnd);
    activePassQuery = nullptr;
}

const PassStats& D3D11PassTracker::getPassStats()
{
    aggregate.totalFrameTime = 0.f;
    aggregate.frame = 0;
    aggregate.stats.clear();

    // Scan to find the highest frame number we can use
    uint64_t reportingFrame = 0;
    for (const auto& pass : passInfo)
    {
        const PassInformation& info = pass.second;
        reportingFrame = max(reportingFrame, info.lastFrameUsed);
    }
    // Generate frame averages
    aggregate.frame = reportingFrame;
    for (const auto& pass : passInfo)
    {
        const PassInformation& info = pass.second;
        if (info.lastFrameUsed == reportingFrame)
        {
            float average = 0.f;
            for (const float frameTime : info.frameTime)
                average += frameTime;
            average /= info.frameTime.size();

            aggregate.stats.push_back({pass.first, average});
            aggregate.totalFrameTime += average;
        }
    }
    return aggregate;
}

} // namespace Graphics
} // namespace Engine