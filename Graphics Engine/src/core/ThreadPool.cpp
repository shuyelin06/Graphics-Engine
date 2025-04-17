#include "ThreadPool.h"

namespace Engine {
ThreadPool* ThreadPool::threadpool = nullptr;

void ThreadPool::InitializeThreadPool() { threadpool = new ThreadPool(); }
ThreadPool* ThreadPool::GetThreadPool() { return threadpool; }
void ThreadPool::DestroyThreadPool() { delete threadpool; }

ThreadPool::ThreadPool() {
    finished = false;

    // Create my thread workers. These will execute the
    // executeWorker function.
    for (int i = 0; i < NUM_THREADS; i++) {
        active[i].store(false);
    }

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

// JobCount:
// Returns the number of jobs the thread pool currently has
int ThreadPool::countPendingJobs() {
    int result;
    {
        std::unique_lock<std::mutex> lock(job_mutex);
        result = job_queue.size();
    }
    return result;
}

// ActiveThreads:
// Returns the # active threads
int ThreadPool::countActiveWorkers() {
    int count = 0;

    for (auto& flag : active) {
        if (flag.load())
            count++;
    }

    return count;
}

// ExecuteWorker:
// Worker function. Workers will work indefinitely until
// the finish boolean is toggled.
void ThreadPool::executeWorker(int index) {

    while (true) {
        // Grab the first job in the queue.
        // We use mutexes to synchronize our threads
        // so that we have no race conditions.
        std::function<void()> cur_job;
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
            cur_job = job_queue.front();
            job_queue.pop();
        }

        // Execute this job. Mark as active while executing.
        active[index].store(true);
        cur_job();
        active[index].store(false);
    }
}

} // namespace Engine