#include "include/util/Profiling.h"

#include <assert.h>
#include <unordered_map>
#include <vector>

#include "src/dependencies/UIBridge.h"

using namespace std::chrono;

namespace Profiling
{
ProfileScope::ProfileScope(ProfileScopeInfo& info,
                           const std::string_view& scopeName)
    : infoPtr(&info)
{
    infoPtr->scopeName = scopeName;

    startTime = std::chrono::steady_clock::now();

    // Signal to ProfileScopeContainer for proper nesting
    profilingScopes.enterScope(infoPtr);
}

ProfileScope::~ProfileScope()
{
    // Signal to ProfileScopeContainer for proper nesting
    profilingScopes.exitScope(infoPtr);

    // Record time spent
    const TimePoint endTime = std::chrono::steady_clock::now();
    const duration<double> seconds = endTime - startTime;
    infoPtr->milliseconds = seconds.count() * 1000.f;

    // Complete
    infoPtr->initialized = true;
}

void ProfileSampleSmoother::addSample(float sample)
{
    // Ring buffer not full. Push to end
    if (size < samples.size())
    {
        size++;
        samples[head++] = sample;
        currentTotal += sample;
    }
    // Ring buffer full. Wrap!
    else
    {
        head = (head + 1) % samples.size();
        currentTotal = currentTotal - samples[head] + sample;
        samples[head] = sample;
    }
}
float ProfileSampleSmoother::getAverage() const { return currentTotal / size; }

ProfileScopeInfo& ProfileScopeContainer::initScope()
{
    assert(scopesSize < scopes.size());
    if (scopesSize < scopes.size())
    {
        scopes[scopesSize].index = scopesSize;
        return scopes[scopesSize++];
    }
    // Overflow: Silently overwrite the last scope
    else
    {
        return scopes[scopesSize];
    }
}

void ProfileScopeContainer::reset()
{
    for (int i = 0; i < scopesSize; i++)
    {
        ProfileScopeInfo& scope = scopes[i];
        scope.initialized = false;
    }
    scopesSize = 0;
}

void ProfileScopeContainer::enterScope(ProfileScopeInfo* curScope)
{
    assert(currentScope == nullptr || currentScope != curScope);

    // For new scope we are entering, mark parent
    curScope->parent = currentScope;

    // Update current scope
    currentScope = curScope;
}

void ProfileScopeContainer::exitScope(ProfileScopeInfo* curScope)
{
    assert(currentScope != nullptr && currentScope == curScope);
    currentScope = curScope->parent;
}

#if defined(UTILITY_IMGUI_ENABLED)
#define IMGUI_SPACING(spacing)                                                 \
    do                                                                         \
    {                                                                          \
        ImGui::Dummy(ImVec2(spacing * 10.f, 0));                               \
        ImGui::SameLine();                                                     \
    } while (false);

using ScopeArray = ProfileScopeContainer::ScopeArray;
using SampleArray = ProfileScopeContainer::SampleArray;

static std::vector<uint16_t> rootScopes{};
static std::unordered_map<uint16_t, std::vector<uint16_t>> scopeChildren{};

static void doScopeImGui(uint16_t scopeIdx,
                         float parentMs,
                         const ScopeArray& scopes,
                         const SampleArray& samples,
                         int depth)
{
    ImGui::TableNextRow();

    const ProfileScopeInfo& scope = scopes[scopeIdx];
    const ProfileSampleSmoother& sample = samples[scopeIdx];

    // Render Node + Label
    ImGui::TableSetColumnIndex(0);
    ImGuiTreeNodeFlags nodeFlags =
        ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
    if (scopeChildren[scopeIdx].empty())
    {
        // Leaf nodes don't need a collapsing arrow and don't push to the ID stack
        nodeFlags |=
            ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    const bool isNodeOpen =
        ImGui::TreeNodeEx(reinterpret_cast<const void*>(&scope), nodeFlags,
                          "%s", scope.scopeName.data());

    const float ms = sample.getAverage();
    ImGui::TableSetColumnIndex(1);
    IMGUI_SPACING(depth);
    ImGui::Text("%.3f ms", ms);

    ImGui::TableSetColumnIndex(2);
    IMGUI_SPACING(depth);
    ImGui::Text("%.2f%%", 100.f * ms / parentMs);

    if (isNodeOpen && !scopeChildren[scopeIdx].empty())
    {
        // Aggregate child timings for % calculation
        float childMsTotal = 0.f;
        for (const uint16_t childIdx : scopeChildren[scopeIdx])
            childMsTotal += samples[childIdx].getAverage();
        // Traverse into children
        for (const uint16_t childIdx : scopeChildren[scopeIdx])
            doScopeImGui(childIdx, childMsTotal, scopes, samples, depth + 1);

        ImGui::TreePop(); // Only needed if the node was not a leaf
    }
};
#endif

void Profiling::DoProfilerImgui()
{
#if defined(UTILITY_IMGUI_ENABLED)
    rootScopes.clear();
    scopeChildren.clear();

    // Build my scope tree
    ScopeArray& scopes = profilingScopes.getScopes();
    SampleArray& samples = profilingScopes.getSamples();

    float rootMsTotal = 0.f;
    for (uint16_t idx = 0; idx < profilingScopes.getScopesSize(); idx++)
    {
        ProfileScopeInfo& scope = scopes[idx];
        if (!scope.initialized)
            continue;

        ProfileSampleSmoother& sample = samples[idx];
        sample.addSample(scope.milliseconds);

        if (scope.parent == nullptr)
        {
            rootScopes.push_back(scope.index);
            rootMsTotal += sample.getAverage();
        }
        else
            scopeChildren[scope.parent->index].push_back(scope.index);
    }

    // Render my scopes to ImGui
    if (ImGui::BeginTable("Profiler Timings", 3))
    {
        ImGui::TableSetupColumn("Scope");
        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("% of Parent");
        ImGui::TableHeadersRow();

        for (uint16_t root : rootScopes)
        {
            doScopeImGui(root, rootMsTotal, scopes, samples, 0);
        }

        ImGui::EndTable();
    }
#endif // UTILITY_IMGUI_ENABLED
}

} // namespace Profiling