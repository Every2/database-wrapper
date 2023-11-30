#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <cassert>
#include <memory>
#include <iostream>

struct Work {
    std::function<void(void *)> function {nullptr};
    std::shared_ptr<void> arg;
};

struct ThreadPool {
    std::vector<std::thread> threads;
    std::deque<Work> queue;
    std::mutex mutex;
    std::condition_variable not_empty;
};

void thread_pool_init(std::shared_ptr<ThreadPool> threadpool, size_t num_threads);
void thread_pool_queue(std::shared_ptr<ThreadPool> threadpool, std::function<void(void *)> function, std::shared_ptr<void> arg);