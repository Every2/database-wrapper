#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <string_view>
#include <iostream>

static void msg(std::string_view msg) {
    std::cerr << msg << '\n';
}

static void die(std::string_view msg) {
    int err {errno};
    std::cerr << err << msg << '\n';
}

static void fd_set_nb(int fd) {
    errno = 0;
    int flags {fcntl(fd, F_GETFL, 0)};
    if (errno) {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;
    errno = 0;
    static_cast<void>(fcntl(fd, F_SETFL, flags));
    if (errno) {
        die("fcntl error");
    }
}

constexpr size_t k_max_msg {4096};

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};


struct Conn {
    int fd {-1};
    uint32_t state {0};
    std::string rbuf;
    std::string wbuf;
    size_t rbuf_size {rbuf.size()};
    size_t wbuf_size {wbuf.size()};
    size_t wbuf_sent {0};    
};

static int32_t accept_new_conn(std::vector <Conn *>& fd2coon, int fd)  {
    
}