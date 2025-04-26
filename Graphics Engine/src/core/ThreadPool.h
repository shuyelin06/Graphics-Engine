#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

constexpr int NUM_THREADS = 7;

namespace Engine {
// Class ThreadPool:
// Implements a thread pool, which uses the same N threads
// to execute a variety of jobs asynchronously.
// Code taken from
// https://dev.to/ish4n10/making-a-thread-pool-in-c-from-scratch-bnm
/*
Example Usage with std::future

std::vector<std::future<int>> results;
for (int i = 0; i < 8; i++) {
    auto future = pool.scheduleJob([i] { return i + 1; });
    results.emplace_back(std::move(future));
}

for (int i = 0; i < 8; i++) {
    int result = results[i].get();
}
*/
class ThreadPool {
  private:
    // Singleton Instance
    static ThreadPool* threadpool;

    // Fields
    std::thread workers[NUM_THREADS];
    std::atomic<bool> active[NUM_THREADS];

    std::queue<std::function<void()>> job_queue;
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
    static ThreadPool* GetThreadPool();
    static void DestroyThreadPool();

    // Get inactive job pool size
    int countPendingJobs();
    // Get # active threads
    int countActiveWorkers();
    // Get # total threads

    // Schedule a new job
    template <typename F, typename... Args>
    auto scheduleJob(F&& f, Args&&... args)
        -> std::future<decltype(f(args...))>;
};

// ScheduleJob:
// Given a function and its arguments, binds the arguments and converts it into
// an std::function so that another thread can execute it.
template <typename F, typename... Args>
inline auto ThreadPool::scheduleJob(F&& f, Args&&... args)
    -> std::future<decltype(f(args...))> {
    // Bind my function with it's arguments
    auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    // Make the function pointer copyable, by making it a shared pointer
    // (so it can be saved to the queue)
    auto encapsulated_ptr =
        std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

    // Add to our queue, locking temporarily to avoid race conditions
    std::future<std::invoke_result_t<F, Args...>> future_object =
        encapsulated_ptr->get_future();
    {
        std::unique_lock<std::mutex> lock(job_mutex);
        job_queue.emplace([encapsulated_ptr]() {
            (*encapsulated_ptr)(); // execute the fx
        });
    }

    // Notify one of our workers
    condition.notify_one();

    return future_object;
}

} // namespace Engine