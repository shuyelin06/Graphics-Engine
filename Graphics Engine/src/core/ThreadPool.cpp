#include "ThreadPool.h"

namespace Engine {
ThreadPool* ThreadPool::threadpool = nullptr;

void ThreadPool::InitializeThreadPool() { threadpool = new ThreadPool(); }
void ThreadPool::DestroyThreadPool() {
    delete threadpool;
    threadpool = nullptr;
}

ThreadPool::ThreadPool() {
    finished = false;

    // Create my thread workers. These will execute the
    // executeWorker function.
    numActive.store(0, std::memory_order_relaxed);
    for (int i = 0; i < NUM_THREADS; i++) {
        workers[i] = std::thread(&ThreadPool::executeWorker, this, i);
    }
}

ThreadPool::~ThreadPool() {
    // Set finished for the thread pool so it stops
    {
        std::unique_lock<std::mutex> lock(job_mutex);
        finished = true;
    }

    // Notify all workers to stop them
    condition.notify_all();

    // Wait on all workers to finish
    for (int i = 0; i < NUM_THREADS; i++)
        workers[i].join();
}

uint8_t ThreadPool::GetNumberActiveWorkers() {
    return threadpool->numActive.load(std::memory_order_acquire);
}

int ThreadPool::GetNumberPendingJobs() {
    int result;
    {
        std::unique_lock<std::mutex> lock(threadpool->job_mutex);
        result = threadpool->job_queue.size();
    }
    return result;
}

void ThreadPool::ScheduleJob(UniqueFunction function) {
    auto& instance = *threadpool;

    // Add to our queue, locking temporarily to avoid race conditions
    {
        std::unique_lock<std::mutex> lock(instance.job_mutex);
        instance.job_queue.push_back(std::move(function));
    }

    // Notify one of our workers
    instance.condition.notify_one();
}

// ExecuteWorker:
// Worker function. Workers will work indefinitely until
// the finish boolean is toggled.
void ThreadPool::executeWorker(int index) {

    while (true) {
        // Grab the first job in the queue.
        // We use mutexes to synchronize our threads
        // so that we have no race conditions.
        UniqueFunction curJob;
        {
            // Lock the job queue so other threads block
            std::unique_lock<std::mutex> lock(job_mutex);

            // Wait until the ThreadPool is finished, or the job queue
            // has entries
            condition.wait(lock,
                           [this]() { return finished || !job_queue.empty(); });

            // If we finished blocking, then we need to check a few cases.
            // Case 1) ThreadPool is done. Stop execution.
            if (finished)
                break;

            // Case 2) Job queue is empty. Keep waiting.
            if (job_queue.empty())
                continue;

            // Case 3) Job queue has a job. Take this job.
            curJob = std::move(job_queue.front());
            job_queue.pop_front();
        }

        // Execute this job. Mark as active while executing.
        numActive.fetch_add(1, std::memory_order_relaxed);
        curJob();
        numActive.fetch_sub(1, std::memory_order_relaxed);
    }
}

} // namespace Engine