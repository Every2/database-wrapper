#include <winsock2.h>
#include <iostream>
#include <string>
#include <array>

using namespace std::string_literals;

static void die(const std::string msg) {
    std::cout << msg << WSAGetLastError() << '\n';
    abort();
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
    int rv = connect(fd, (const struct sockaddr*)&addr, sizeof(addr));
    if (rv == SOCKET_ERROR) {
        die("connect"s);
    }

    std::string msg {"hello"s};
    send(fd, msg.data(), msg.size(), 0);

    std::array <char,  64> rbuf{};
    int n = recv(fd, rbuf.data(), rbuf.size() - 1, 0);
    if (n < 0) {
        die("read"s);
    }
    std::cout << "server say: "s << rbuf.data() <<  '\n';

    closesocket(fd);
    WSACleanup();

    return 0;
}