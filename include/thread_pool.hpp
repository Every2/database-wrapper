#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <any>
#include <condition_variable>

using function_pointer = void(*)(void *);

struct Work {
    function_pointer function = nullptr;
    std::any *arg = nullptr;
};

struct Threadpool {
    std::vector<std::thread> threads;
    std::deque<Work> queue;
    std::mutex mu;
    std::condition_variable not_empty;
};

void thread_pool_init(Threadpool *thread_pool, size_t number_of_threads);
void thread_pool_queue(Threadpool *thread_pool, function_pointer, std::any *arg);