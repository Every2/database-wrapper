#include "../include/thread_pool.hpp"

static void worker(std::shared_ptr<ThreadPool> threadpool) {
    while (true) {
        std::unique_lock<std::mutex> lock(threadpool->mutex);
        threadpool->not_empty.wait(lock, [&threadpool] { return !threadpool->queue.empty(); });

        Work worker = std::move(threadpool->queue.front());
        threadpool->queue.pop_front();

        lock.unlock();

        worker.function(worker.arg.get());
    }
}

void thread_pool_init(std::shared_ptr<ThreadPool> threadpool, size_t num_threads) {
    assert(num_threads > 0);

    threadpool->threads.resize(num_threads);
    for (size_t i {0}; i < num_threads; ++i) {
        threadpool->threads.at(i) = std::thread(worker, threadpool);
    }
}

void thread_pool_queue(std::shared_ptr<ThreadPool> threadpool, std::function<void(void *)> function, std::shared_ptr<void> arg) {
    Work worker;
    worker.function = std::move(function);
    worker.arg = arg;

    {
        std::unique_lock<std::mutex> lock(threadpool->mutex);
        threadpool->queue.push_back(std::move(worker));
    }

    threadpool->not_empty.notify_one();
}