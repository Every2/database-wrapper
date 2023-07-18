#include <winsock2.h>
#include <iostream>
#include <string>
#include <cassert>
#include <cstring>
#include <vector>
#include <array>

using namespace std::string_literals;

static void msg(const std::string msg) {
    std::cerr << msg << '\n';
}

static void die(const std::string msg) {
    std::cerr << msg << '[' << WSAGetLastError() << ']' << '\n';
    abort();
}

static int32_t read_full(SOCKET fd, std::vector<char>& buf, size_t n) {
    size_t received = 0;
    while (received < n) {
        int rv = recv(fd, buf.data() + received, static_cast<int>(n - received), 0);
        if (rv <= 0) {
            return -1;
        }
        received += rv;
    }
    return 0;
}

static int32_t write_all(SOCKET fd, const std::vector<char>& buf, size_t n) {
    size_t sent = 0;
    while (sent < n) {
        int rv = send(fd, buf.data() + sent, static_cast<int>(n - sent), 0);
        if (rv <= 0) {
            return -1;  
        }
        sent += rv;
    }
    return 0;
}

const size_t k_max_msg = 4096; 

static int32_t send_req(SOCKET fd, const std::string& text) {
    uint32_t len = static_cast<uint32_t>(text.length());
    if (len > k_max_msg) {
        return -1;
    }

    std::vector<char> wbuf(4 + k_max_msg);
    memcpy(wbuf.data(), &len, 4); 
    memcpy(wbuf.data() + 4, text.data(), text.length());
    return write_all(fd, wbuf, 4 + text.length());
}

static int32_t read_res(SOCKET fd) {
    std::vector<char> rbuf(4 + k_max_msg + 1);
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err == SOCKET_ERROR) {
        if (errno == 0) {
            msg("EOF");
        } else {
           msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf.data(), 4);
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    rbuf.resize(len);
    err = read_full(fd, rbuf, len);
    if (err) {
        msg("read() error");
        return err;
    }

    rbuf.push_back('\0');
    std::cout << "server says: " << rbuf.data() << '\n';
    return 0;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        die("WSAStartup failed"s);
    }

    SOCKET fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET) {
        die("socket()"s);
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  
    int rv = connect(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
    if (rv != 0) {
        die("connect"s);
    }

    std::vector<std::string> query_list{"hello1", "hello2", "hello3"};
    for (const auto& query : query_list) {
        int32_t err = send_req(fd, query);
        if (err) {
            goto L_DONE;
        }
    }

    for (size_t i = 0; i < query_list.size(); ++i) {
        int32_t err = read_res(fd);
        if (err) {
            goto L_DONE;
        }
    }

L_DONE:
    closesocket(fd);
    WSACleanup();
    return 0;
    
}
