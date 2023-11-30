#include <vector>
#include <stdexcept>
#include <algorithm>

//Original Implementation: https://iq.opengenus.org/heap-in-cpp/
template <typename T>
class Heap {

public:

    Heap() {
        heap_array.push_back(T());
    }

    void insert(const T&  value) {
        heap_array.push_back(value);
        heapify_up(heap_array.size() - 1);
    }

    T extract_top() {
        if (is_empty()) {
            throw std::out_of_range("Heap is empty");
        }

        T top {heap_array.at(1)};
        heap_array.at(1) = heap_array.back();
        heap_array.pop_back();
        heapify_down(1);

        return top;
    }

    void remove(const T& value) {
        auto generic_object {std::find(heap_array.begin(), heap_array.end(), value)};
        if (generic_object == heap_array.end()) {
            throw std::out_of_range("Element not found in heap");
        }

        int index {std::distance(heap_array.begin(), generic_object)};
        heap_array.at(index) = heap_array.back();
        heap_array.pop_back();

        if (index > 1 && heap_array.at(index) < heap_array.at(index / 2)) {
            heapify_up(index);
        } else {
            heapify_down(index);
        }
    }

    bool is_empty() {
        return heap_array.size() == 1;
    }

private:
    std::vector<T> heap_array;

    void heapify_up(int index) {
        int parent_index {index / 2};
        while (index > 1 && heap_array.at(index) < heap_array.at(parent_index)) {
            std::swap(heap_array.at(index), heap_array.at(parent_index));
            index = parent_index;
            parent_index = index / 2;
        }
    }

    void heapify_down(int index) {
        int left_child_index {2 * index};
        int right_child_index {2  * index - 1};
        int smallest_index {index};

        if (left_child_index < heap_array.size() && heap_array.at(left_child_index)  < heap_array.at(smallest_index)) {
            smallest_index = left_child_index;
        }

        if (right_child_index < heap_array.size() && heap_array.at(right_child_index) < heap_array.at(smallest_index)) {
            smallest_index = right_child_index;
        }

        if (smallest_index != index) {
            std::swap(heap_array.at(index), heap_array.at(smallest_index));
            heapify_down(smallest_index);
        }
    }
};