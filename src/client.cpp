#include <iostream>
#include <vector>
#include <string_view>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <cassert>

#pragma comment(lib, "ws2_32.lib")
static void msg(std::string_view msg) {
    std::cerr << msg << '\n';
}

static void die(std::string_view msg) {
    int err {WSAGetLastError()};
    std::cerr << WSAGetLastError << msg << '\n';
    std::abort();
}

static UINT32 read_full(SOCKET fd, char* buf, size_t n) {
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

static UINT32 write_all(SOCKET fd, const char* buf, size_t n) {
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

static UINT32 send_req(SOCKET fd, const std::string_view text) {
    UINT32 len = static_cast<UINT32>(text.length());
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4); 
    memcpy(wbuf + 4, text.data(), text.length());
    return write_all(fd, wbuf, 4 + text.length());
}

static UINT32 read_res(SOCKET fd) {
    char rbuf[4 + k_max_msg + 1];
    UINT32 err = read_full(fd, rbuf, 4);
    if (err) {
        msg("read() error");
        return err;
    }

    UINT32 len {0};
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

    const char *query_list[3] {"hello1, hello2, hello3"};

    for (size_t i {0}; i < 3; ++i) {
        UINT32 err {send_req(fd, query_list[i])};
        if (err) {
            goto L_DONE;
        }
    }
    for (size_t i {0}; i  < 3; ++i) {
        UINT32 err {read_res(fd)};
        if (err) {
            goto L_DONE;
        }
    }

L_DONE:
    closesocket(fd);
    WSACleanup();
    return 0;
    
}
