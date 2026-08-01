#pragma once

#include <any>
#include <assert.h>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ThreadPool.h"
#include "UniqueFunction.h"

namespace Engine
{
// JobGraph lets you define a sequence of jobs, which are functions which may or
// may not have dependencies with each other. Jobs are automatically scheduled
// when they have no dependencies and where they are scheduled depends on their context:
// - Async: Scheduled in the ThreadPool.
// - Sync: Scheduled in the JobGraph synchronous job queue (must manually call void processSynchronousJobs())
// To share data between jobs, JobPool provides thread-safe access to JobGraphMemoryPool.
// Jobs can move data under unique_ptrs to the pool, and associate it with a string token for other jobs
// to access.
class JobGraphMemoryPool
{
  private:
    // Erase the type under the unique ptrs so that we can store them together
    // in the map.
    struct WrapperBase
    {
        virtual ~WrapperBase() = default;
    };
    template <typename T> struct Wrapper : public WrapperBase
    {
        std::unique_ptr<T> ptr;
        Wrapper(std::unique_ptr<T> ptr)
            : ptr(std::move(ptr))
        {
        }
    };

    std::unordered_map<std::string, std::unique_ptr<WrapperBase>> mMemoryMap;
    std::mutex mMemoryMapLock;

  public:
    JobGraphMemoryPool();
    ~JobGraphMemoryPool();

    template <typename T>
    void store(const std::string& token, std::unique_ptr<T>&& data)
    {
        std::scoped_lock<std::mutex> lock(mMemoryMapLock);
        mMemoryMap[token] = std::make_unique<Wrapper<T>>(std::move(data));
    }

    template <typename T> std::unique_ptr<T> load(const std::string& token)
    {
        std::scoped_lock<std::mutex> lock(mMemoryMapLock);

        auto iter = mMemoryMap.find(token);
        if (iter == mMemoryMap.end())
        {
            return nullptr;
        }

        // Obtain my base class and extract the pointer we need from it.
        WrapperBase* basePtr = iter->second.get();
        Wrapper<T>* wrapperPtr = static_cast<Wrapper<T>*>(basePtr);
        std::unique_ptr<T> ptr = std::move(wrapperPtr->ptr);

        mMemoryMap.erase(iter);
        return ptr;
    }
};

enum class JobGraphContext
{
    kAsync = 0,
    kSynchronous = 1,
};
using JobGraphFunction = UniqueFunction<bool(JobGraphMemoryPool& memoryPool)>;
class JobGraph
{
  private:
    struct JobNode
    {
        // Job and context that this node will execute in
        JobGraphFunction job;
        JobGraphContext context;
        // Number of jobs that this job are waiting on before beginning.
        std::atomic<uint8_t> dependencyCount = 0;
        // Number of jobs that are waiting on this job to complete
        std::vector<JobNode*> dependents;

        JobNode(JobGraphFunction&& func, JobGraphContext context)
            : job(std::move(func))
            , context(context)
            , dependencyCount(0)
            , dependents()
        {
        }
    };
    std::vector<std::unique_ptr<JobNode>> mNodes;

    // Shared memory pool for the job nodes.
    // Job nodes can share memory by moving data into this memory pool
    // for other nodes to take.
    JobGraphMemoryPool mMemoryPool;

    // Synchronous job tasks.
    // The JobGraph owner needs to call processSynchronousJobs()
    // to drain any of these, otherwise the job graph may stall in its
    // execution
    std::deque<ThreadPoolFunction> mSynchronousTasks;
    std::mutex mSynchronousTasksLock;

    // Number of pending jobs. JobGraph cannot be cleaned up until all jobs are
    // done.
    std::atomic<uint8_t> mNumPendingJobs;
    std::atomic<bool> mAbort;

    // Gates against adding to the graph after kickoff
    bool kickedOff = false;

  public:
    using JobID = size_t;
    static constexpr JobID kInvalidJobID = ~0;
    JobGraph();
    ~JobGraph();

    // Setup. Call these on the same thread before kickoff.
    JobID createJob(JobGraphContext context, JobGraphFunction&& func);
    void registerDependency(JobID parentID, JobID dependentID);
    template <typename T>
    void storeMemory(const std::string& token, std::unique_ptr<T>&& data)
    {
        mMemoryPool.store(token, std::move(data));
    }

    // Kickoff! Begin executing jobs in the job graph.
    void kickoff();

    // Query data from the JobGraph. Okay to call anytime
    bool isKickedOff() const;
    bool isDone() const;
    bool isSuccessful() const;
    uint8_t numPendingJobs() const;

    // Process any synchronous jobs in the job graph.
    void processSynchronousJobs();

  private:
    ThreadPoolFunction packageJob(JobNode& node);
    void scheduleJob(JobNode& node);
};

} // namespace Engine