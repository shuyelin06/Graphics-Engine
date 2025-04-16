#pragma once

#include <thread>
#include <vector>

namespace Engine {
class ThreadPool {
  private:
    std::vector<std::thread> threads;

  public:
    ThreadPool(int num_threads);
};

} // namespace Engine