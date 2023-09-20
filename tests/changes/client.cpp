#include <iostream>
#include <vector>
#include <string_view>
#include <WinSock2.h>
#include <cstdlib>
#include <cassert>
#include <cstdint>
#include <array>

static void msg(std::string_view msg) {
    std::cerr << msg << '\n';
}

static void die(std::string_view msg) {
    int err {WSAGetLastError()};
    std::cerr << err << msg << '\n';
    std::abort();
}

static uint32_t read_full(SOCKET fd, char* buf, size_t n) {
    while (n > 0) {
        int rv {recv(fd, buf, n, 0)};
        if (rv <= 0) {
            return -1;
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buf += rv;
    }
    return 0;
}

static uint32_t write_all(SOCKET fd, const char* buf, size_t n) {
    while (n > 0) {
        int rv {send(fd, buf, n, 0)};
        if (rv <= 0) {
            return -1;  
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buf += rv;
    }
    return 0;
}

const size_t k_max_msg = 4096; 

static uint32_t send_req(SOCKET fd, const std::string text) {
    uint32_t len = static_cast<uint32_t>(text.length());
    if (len > k_max_msg) {
        return -1;
    }

    len = static_cast<uint32_t>(text.length());

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4); 
    memcpy(wbuf + 4, text.data(), len);

    
    return write_all(fd, wbuf, 4 + len);
}

static uint32_t read_res(SOCKET fd) {
    char rbuf[4 + k_max_msg - 1];
    uint32_t err = read_full(fd, rbuf, 4);
    if (err) {
        msg("read() error");
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

    rbuf[4 + len] = '\0';
    std::cout << "server says: " << &rbuf[4] << '\n';
    return 0;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        die("WSASTARTUP failed");
    }

    SOCKET fd {socket(AF_INET,  SOCK_STREAM, 0)};
    if (fd == INVALID_SOCKET) {
        die("socket()");
    }

    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  
    int rv {connect(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr))};
    if (rv != 0) {
        die("connect");
    }

    
    std::array <std::string, 3> query_list {"hello1", "hello2", "hello3"};

    for (size_t i {0}; i < query_list.size(); ++i) { 
        uint32_t err {send_req(fd, query_list[i])};
        if (err) {
            goto L_DONE;
        }
    }

    for (size_t i {0}; i  < 3; ++i) {
        uint32_t err {read_res(fd)};
        if (err) {
            goto L_DONE;
        }
    }

L_DONE:
    closesocket(fd);
    WSACleanup();
    return 0;
    
}