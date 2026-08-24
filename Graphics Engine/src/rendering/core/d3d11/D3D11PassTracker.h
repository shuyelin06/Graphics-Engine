#pragma once

#include <array>
#include <string>
#include <unordered_map>

#include "../Device.h"

#include "rendering/Direct3D11.h"

namespace Engine
{
namespace Graphics
{
// Number of data points we hold on to per pass, so we can smooth
// the output
constexpr int kNumActiveQueries = 3;
constexpr int kPassTrackingWindow = 4;

class D3D11PassTracker
{
  private:
    struct PassQuery
    {
        std::string_view passName;
        ID3D11Query* queryBegin = nullptr;
        ID3D11Query* queryEnd = nullptr;
    };
    struct FrameQuery
    {
        uint64_t frame = UINT64_MAX;

        // Disjoint queries so we can convert to timestamp information
        ID3D11Query* disjointQuery;

        // Queries done this frame, per pass. We will reuse the vector
        // So we will maintain a head to know how many PassQueries to consider.
        std::vector<PassQuery> passes;
        size_t passesHead;
    };
    std::array<FrameQuery, kNumActiveQueries> queries{};
    size_t queriesHead = 0;

    uint64_t activeFrame = 0;
    FrameQuery* activeFrameQuery = nullptr;
    PassQuery* activePassQuery = nullptr;

    struct PassInformation
    {
        uint64_t lastFrameUsed = 0;
        std::array<float, kNumActiveQueries> frameTime = {0.f};
        size_t frameTimeHead = 0;
    };
    std::unordered_map<std::string_view, PassInformation> passInfo;
    PassStats aggregate;

  public:
    D3D11PassTracker(ID3D11Device* device);
    ~D3D11PassTracker();

    void beginFrame(uint64_t frameNumber, ID3D11DeviceContext* context);
    void endFrame(ID3D11DeviceContext* context);

    void beginPass(const char* passName,
                   ID3D11Device* device,
                   ID3D11DeviceContext* context);
    void endPass(ID3D11DeviceContext* context);

    const PassStats& getPassStats();

  private:
    void harvestFrameQuery(const FrameQuery& frameQuery,
                           ID3D11DeviceContext* context);
};

} // namespace Graphics
} // namespace Engine