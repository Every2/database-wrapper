#include <winsock2.h>
#include <iostream>
#include <string>
#include <array>

using namespace std::string_literals;

static void msg(const std::string msg) {
    std::cout << msg;
}

static void die(const std::string msg) {
    std::cout << msg << WSAGetLastError() << '\n';
    abort();
}

static void do_something(SOCKET connfd) {
    std::array <char,  64> rbuf{};
    int n = recv(connfd, rbuf.data(), rbuf.size() - 1, 0);
    if (n < 0) {
        msg("read() error"s);
        return;
    }
    std::cout << "client say: "s << rbuf.data() <<  '\n';

    std::string wbuf = "world"s;
    send(connfd, wbuf.data(), wbuf.size(), 0);
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

        do_something(connfd);
        closesocket(connfd);
    }

    closesocket(fd);
    WSACleanup();

    return 0;
}