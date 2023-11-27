#include <cstdint>
#include <string>
#include <list>

template <typename T>
struct ConnectionInfo {
    int file_descriptor {-1};
    uint32_t state {0};
    std::string read_buffer{};
    std::string write_buffer{};
    uint64_t idle_start {0};
    std::list<T> idle_list{};
};

enum class ConnectionState {
    request_state = 0,
    response_state = 1,
    end_state = 2
};

