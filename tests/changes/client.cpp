#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <string>
#include <vector>
#include <string_view>


static void msg(std::string_view msg) {
    std::cerr << stderr << msg;
}    

static void die(std::string_view msg) {
    int err {errno};
    std::cout << stderr << err << msg;
    std::abort();
}

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buf += rv;
    }
    return 0;
}

const size_t k_max_msg {4096};

static int32_t send_req(int fd, const std::vector<std::string>& cmd) {
    uint32_t len {4};
    for (const std::string& s : cmd) {
        len += 4 + s.size();
    }
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(&wbuf[0], &len, 4);  // assume little endian
    uint32_t n {static_cast<uint32_t>(cmd.size())};
    memcpy(&wbuf[4], &n, 4);
    size_t cur {8};
    for (const std::string& s : cmd) {
        uint32_t p {static_cast<uint32_t>(s.size())};
        memcpy(&wbuf[cur], &p, 4);
        memcpy(&wbuf[cur + 4], s.data(), s.size());
        cur += 4 + s.size();
    }
    return write_all(fd, wbuf, 4 + len);
}

static int32_t read_res(int fd) {
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err {read_full(fd, rbuf, 4)};
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len {0};
    memcpy(&len, rbuf, 4);  
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    uint32_t rescode {0};
    if (len < 4) {
        msg("bad response");
        return -1;
    }
    memcpy(&rescode, &rbuf[4], 4);
    std::cout << "Server says: " << '[' << rescode << ']' << std::string(&rbuf[8], len - 4) << '\n';
    return 0;
}

int main(int argc, char **argv) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); 
    int rv {connect(fd, (const struct sockaddr *)&addr, sizeof(addr))};
    if (rv) {
        die("connect");
    }

    std::vector<std::string> cmd;
    for (int i = 1; i < argc; ++i) {
        cmd.push_back(argv[i]);
    }
    int32_t err {send_req(fd, cmd)};
    if (err) {
        goto L_DONE;
    }
    err = read_res(fd);
    if (err) {
        goto L_DONE;
    }

L_DONE:
    close(fd);
    return 0;
}