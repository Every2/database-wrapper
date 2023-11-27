#include <cstdint>
#include <string>

struct ConnectionInfo {
    int file_descriptor {-1};
    uint32_t state {0};
    std::string read_buffer{};
    std::string write_buffer{};
};

enum ConnectionState {
    request_state = 0,
    response_state = 1,
    end_state = 2
};

enum response {
    response_ok = 0,
    response_error = 1,
    response_nx = 2,
};
