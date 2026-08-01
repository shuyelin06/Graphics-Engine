#include "JobGraph.h"

namespace Engine {
JobGraphMemoryPool::JobGraphMemoryPool() = default;
JobGraphMemoryPool::~JobGraphMemoryPool() = default;

JobGraph::JobGraph() = default;
JobGraph::~JobGraph() {
    while (isKickedOff() && !isDone()) {
        // Drain the syn
        processSynchronousJobs();
        if (!isDone())
            std::this_thread::yield();
    }
}

JobGraph::JobID JobGraph::createJob(JobGraphContext context,
                                    JobGraphFunction&& func) {
    if (kickedOff)
        throw std::runtime_error("JobGraph Kicked Off Already!");

    JobID id = mNodes.size();
    mNodes.emplace_back(std::make_unique<JobNode>(std::move(func), context));
    return id;
}

void JobGraph::registerDependency(JobID parentID, JobID dependentID) {
    if (kickedOff)
        throw std::runtime_error("JobGraph Kicked Off Already!");

    assert(parentID < mNodes.size() && dependentID < mNodes.size());

    auto& parent = mNodes[parentID];
    auto& dependent = mNodes[dependentID];

    // Register dependency.
    // 1) Parent tracks the dependent, so when it is done executing it knows
    //    which dependents' counters to decrement.
    // 2) Dependent counter increments so we know we cannot immediately kick it
    //    off.
    // Memory order can be relaxed as this is setup.
    parent->dependents.push_back(dependent.get());
    dependent->dependencyCount.fetch_add(1, std::memory_order_relaxed);
}

void JobGraph::kickoff() {
    kickedOff = true;

    // TODO: Validation

    // Iterate through my job nodes and figure out which ones don't have dependencies
    // We must gather them first then schedule. Otherwise, we may get a double scheduling issue
    // where during kickoff, a async job finishes making a later (yet to be processed) job
    // ready. So both the async job and kickoff schedule this job.
    std::vector<JobNode*> rootNodes;

    mNumPendingJobs.store(mNodes.size(), std::memory_order_relaxed);
    mAbort.store(false, std::memory_order_relaxed);

    for (auto& node : mNodes) {
        // We have not yet kicked off any jobs yet. So memory order can be relaxed
        // (the only thread executing is the setup thread)
        if (node->dependencyCount.load(std::memory_order_relaxed) == 0) {
            rootNodes.push_back(node.get());
        }
    }

    for (JobNode* node : rootNodes) {
        scheduleJob(*node);
    }
}

bool JobGraph::isKickedOff() const { return kickedOff; }
uint8_t JobGraph::numPendingJobs() const {
    return mNumPendingJobs.load(std::memory_order_relaxed);
}
bool JobGraph::isDone() const {
    return mNumPendingJobs.load(std::memory_order_relaxed) == 0 ||
           mAbort.load();
}
bool JobGraph::isSuccessful() const { return !mAbort.load(); }

void JobGraph::processSynchronousJobs() {
    bool empty = false;

    while (!empty) {
        ThreadPoolFunction task;
        empty = true;

        {
            std::scoped_lock<std::mutex> lock(mSynchronousTasksLock);
            if (!mSynchronousTasks.empty()) {
                empty = false;
                task = std::move(mSynchronousTasks.front());
                mSynchronousTasks.pop_front();
            }
        }

        if (!empty) {
            task();
        }
    }
}

ThreadPoolFunction JobGraph::packageJob(JobNode& jobNode) {
    ThreadPoolFunction threadPoolFunction = [this, &jobNode]() {
        // Validate that an abort has not yet happened
        if (mAbort.load(std::memory_order_acquire))
            return;

        // First, execute my job.
        const bool success = jobNode.job(mMemoryPool);
        if (!success)
            mAbort.store(true, std::memory_order_release);

        // Iterate through dependents and decrement counters
        for (JobNode* dependent : jobNode.dependents) {
            // For any dependents with a counter of 0, we can kick it off!
            // Memory order must be both acquire and release.
            // - Release (Write): When we write to the counter, we guarantee
            //   that other threads can see any changes we may have made to the
            //   memory pool
            // - Acquire (Read): When we read from the counter. We guarantee we
            //   pull in any releases (memory pool writes) from other threads.
            // That way the final thread kicking off the next job will guarantee
            // that the next job has all the memory it needs.
            if (dependent->dependencyCount.fetch_sub(
                    1, std::memory_order_acq_rel) == 1) {
                scheduleJob(*dependent);
            }
        }

        // When done signal this to job graph
        mNumPendingJobs.fetch_sub(1, std::memory_order_relaxed);
    };
    return threadPoolFunction;
}

void JobGraph::scheduleJob(JobNode& node) {
    assert(node.dependencyCount.load(std::memory_order_relaxed) == 0);
    // 1) If the dependent's context is synchronous, push to
    // JobGraph synchronous queue
    // 2) Otherwise, schedule in ThreadPool
    if (node.context == JobGraphContext::kSynchronous) {
        std::scoped_lock<std::mutex> lock(mSynchronousTasksLock);
        mSynchronousTasks.push_back(std::move(packageJob(node)));
    } else if (node.context == JobGraphContext::kAsync) {
        ThreadPool::ScheduleJob(std::move(packageJob(node)));
    } else {
        throw std::runtime_error("Unrecognized Context");
    }
}

} // namespace Engine