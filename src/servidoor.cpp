#include <winsock2.h>
#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <cassert>
#include <cstring>

using namespace std::string_literals;

const size_t k_max_msg = 4096; 

static void msg(const std::string msg) {
    std::cout << msg;
}

static void die(const std::string msg) {
    std::cout << msg << WSAGetLastError() << '\n';
    abort();
}

static int32_t read_full(int fd, std::vector<char> buf, size_t n) {
    size_t index = 0;

    while (n > 0) 
    {
        int rv = recv(fd, buf.data() + index, n, 0);
        if (rv <= 0) 
        {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= static_cast<size_t>(rv);
        index += rv;
    }

    return 0;
}

static int32_t write_all(int fd, const std::vector<char>& buf, int n) {
    size_t index = 0;

    while (n > 0)
    {
        int rv = send(fd, buf.data() + index, n, 0);
        if (rv <= 0) 
        {
            return -1;
        }

        assert((size_t)rv <= n);
        n -= static_cast<size_t>(rv);
        index += rv;

    }

    return 0;
}

static int32_t one_request(int fd) {
    std::vector<char> rbuf(4 + k_max_msg + 1);
    errno = 0;
    int32_t err = read_full(fd, rbuf, rbuf.size());
    if (err) 
    {
        if (errno == 0) 
        {
            msg("EOF");
        }
        else
        {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    std::memcpy(&len, rbuf.data(), rbuf.size());
    if (len > k_max_msg) 
    {
        msg("too long");
        return -1;
    }

    err = read_full(fd, rbuf, len);
    if (err) 
    {
        msg("read() error");
        return err;
    }

    rbuf[4 + len] = '\0';
    std::string clientMsg(rbuf.begin(), rbuf.end());
    std::cout << "Client Say " << clientMsg << '\n';

    std::string reply{"world"s};
    std::vector<char> wbuf(4 + sizeof(reply));
    len = (uint32_t)wbuf.size();
    std::memcpy(wbuf.data(), &len, 4);
    std::memcpy(wbuf.data(), reply.c_str(), len); 
    return write_all(fd, wbuf, 4 + len);
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

    
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof(val));

   
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int rv = bind(fd, (const sockaddr*)&addr, sizeof(addr));
    if (rv == SOCKET_ERROR) {
        die("bind()"s);
    }

  
    rv = listen(fd, SOMAXCONN);
    if (rv == SOCKET_ERROR) {
        die("listen()"s);
    }

    while (true) {
        struct sockaddr_in client_addr = {};
        int socklen = sizeof(client_addr);
        SOCKET connfd = accept(fd, (struct sockaddr*)&client_addr, &socklen);
        if (connfd == INVALID_SOCKET) {
            continue; 
        }

        while (true) {
            int32_t err = one_request(connfd);
            if (err) {
                break;
            }
        }

        closesocket(connfd);
    }

    closesocket(fd);
    WSACleanup();

    return 0;
}