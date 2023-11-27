#include <queue>
#include <mutex>
#include <iostream>

template <typename T>
class Heap {

public:
    H
    void insert(T element) {
        std::lock_guard<std::mutex> lock(mutex_data);
        heap.push(element);
    };

    void remove() {
        std::lock_guard<std::mutex> lock(mutex_data);
        if (heap.empty()) throw std::out_of_range("Queue is empty");
        T element {heap.top()};
        heap.pop;
        return element
    };

private:
    std::priority_queue<T> heap;
    std::mutex mutex_data;
};