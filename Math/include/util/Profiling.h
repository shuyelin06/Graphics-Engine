#pragma once

#include <array>
#include <chrono>
#include <string_view>

#define ENABLE_PROFILING

// Profiler Tool:
// Exposes a PROFILE_SCOPE API that can be used. Include it at the top of any
// scope for the profiler tool to automatically profile the scope.
// Some key notes:
// 1) Scopes are stored per thread. For now only the main thread scopes are reported.
// 2) Every invocation of a function generates its own scope
// TODO Add profiling for other threads
namespace Profiling
{
class ProfileSampleSmoother
{
  private:
    static constexpr int kRingBufferSize = 32;
    std::array<float, kRingBufferSize> samples{};
    uint16_t head = 0;
    uint16_t size = 0;

    float currentTotal = 0.f;

  public:
    ProfileSampleSmoother() = default;

    void addSample(float sample);
    float getAverage() const;
};
struct ProfileScopeInfo
{
    std::string_view scopeName{};
    float milliseconds = 0.f;

    ProfileScopeInfo* parent = nullptr;
    uint16_t index = 0xFFFF;

    bool initialized = false;
};
class ProfileScope
{
  private:
    using TimePoint = std::chrono::steady_clock::time_point;

    ProfileScopeInfo* infoPtr;
    TimePoint startTime;

  public:
    ProfileScope(ProfileScopeInfo& info, const std::string_view& scopeName);
    ~ProfileScope();
};
class ProfileScopeContainer
{
  public:
    static constexpr int kMaxProfileScopes = 512;
    using ScopeArray = std::array<ProfileScopeInfo, kMaxProfileScopes>;
    using SampleArray = std::array<ProfileSampleSmoother, kMaxProfileScopes>;

    ProfileScopeContainer() = default;

    ProfileScopeInfo& initScope();
    void reset();

    void enterScope(ProfileScopeInfo* curScope);
    void exitScope(ProfileScopeInfo* curScope);

    SampleArray& getSamples() { return samples; };
    ScopeArray& getScopes() { return scopes; }
    uint16_t getScopesSize() const { return scopesSize; }

  private:
    ScopeArray scopes;
    SampleArray samples;
    uint16_t scopesSize = 0;

    ProfileScopeInfo* currentScope = nullptr;
};

// Define a thread local copy of ProfileScopeContainer
inline thread_local ProfileScopeContainer profilingScopes;

void DoProfilerImgui();

} // namespace Profiling

// User-Facing Macro
// Uses the thread local profile scope container and creates a scope object from it.
// Note that the scope object has a static lifetime so that we reuse this object every function
// invocation.
#if defined(ENABLE_PROFILING)
#define PROFILE_RESET() Profiling::profilingScopes.reset();
#define PROFILE_SCOPE(name)                                                    \
    Profiling::ProfileScopeInfo& scopeInfo =                                   \
        Profiling::profilingScopes.initScope();                                \
    Profiling::ProfileScope profileScope(scopeInfo, name);
#else
#define PROFILE_SCOPE(name) ;
#define PROFILE_RESET() ;
#endif // ENABLE_PROFILING