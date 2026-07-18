#pragma once

#include <atomic>
#include <deque>
#include <mutex>
#include <thread>

#include "UniqueFunction.h"

constexpr int NUM_THREADS = 7;

namespace Engine {
// Class ThreadPool:
// Implements a thread pool, which uses the same N threads
// to execute a variety of jobs asynchronously.
// Uses a custom UniqueFunction type to allow for lambdas that are move only
// (e.g. lambdas that have ownership of unique ptrs)
class ThreadPool {
  private:
    // Singleton Instance
    static ThreadPool* threadpool;

    // Fields
    std::thread workers[NUM_THREADS];
    std::atomic<uint8_t> numActive;

    std::deque<UniqueFunction> job_queue;
    bool finished;

    // Synchronization on the job_queue
    std::mutex job_mutex;
    // Forces threads to wait if there are no jobs
    std::condition_variable condition;

    // Worker execute function
    void executeWorker(int index);

    ThreadPool();
    ~ThreadPool();

  public:
    static void InitializeThreadPool();

    static uint8_t GetNumberActiveWorkers();
    static int GetNumberPendingJobs();
    static void ScheduleJob(UniqueFunction function);

    static void DestroyThreadPool();
};

} // namespace Engine