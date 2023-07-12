#include <winsock2.h>
#include <iostream>
#include <string>
#include <cassert>
#include <cstring>
#include <vector>
#include <array>

using namespace std::string_literals;

const size_t k_max_msg = 4096; 

static void die(const std::string msg) {
    std::cout << msg << WSAGetLastError() << '\n';
    abort();
}

static void msg(const std::string msg) {
    std::cout << msg;
}

static int32_t read_full(SOCKET fd, std::vector<char>& buf, size_t n) {
    buf.resize(n);
    size_t bytesRead = 0;
    while (bytesRead < n) {
        //&buf[bytesWritten]: The pointer to the buffer containing the data to be sent. It specifies the starting address of the portion of the buffer that is yet to be sent.
        //n - bytesWritten: The remaining number of bytes to be sent from the buffer.
        int rv = recv(fd, &buf[bytesRead], n - bytesRead, 0);
        if (rv <= 0) {
            return -1; 
        }
        bytesRead += rv;
    }
    return 0;
}

static int32_t write_all(SOCKET fd, const std::vector<char>& buf, size_t n) {
    size_t bytesWritten = 0;
    while (bytesWritten < n) {
        int rv = send(fd, &buf[bytesWritten], n - bytesWritten, 0);
        if (rv <= 0) {
            return -1; 
        }
        bytesWritten += rv;
    }
    return 0;
}

static int32_t send_req(SOCKET fd, const std::string text) {
    uint32_t len = (uint32_t)text.size();
    if (len > k_max_msg) {
        return -1;
    }

    std::vector<char> wbuf(4 + k_max_msg);
    std::memcpy(wbuf.data(), &len, 4);
    std::memcpy(&wbuf[4], text.data(), len);
    return write_all(fd, wbuf, 4 + len);
}

static int32_t read_res(SOCKET fd) {
    std::vector<char> rbuf(4 + k_max_msg + 1);
    int32_t err = read_full(fd, rbuf, 4);
    if (err == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();
        if (errorCode == 0) {
            msg("EOF");
        } else {
           msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    std::memcpy(&len, rbuf.data(), 4);
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    err = recv(fd, &rbuf[4], len, 0);
    if (err == SOCKET_ERROR) {
        msg("recv() error");
        return err;
    }

    rbuf[4 + len] = '\n';
    std::string serverMsg{rbuf.begin(), rbuf.end() + 4 + len};
    std::cout << "server says: " <<  serverMsg << '\n';
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
    int rv = connect(fd, (const struct sockaddr*)&addr, sizeof(addr));
    if (rv == SOCKET_ERROR) {
        die("connect"s);
    }

    std::array<std::string, 3> query_list{"hello1", "hello2", "hello3"};
    for (size_t i = 0; i < query_list.size(); ++i) {
        int32_t err = send_req(fd, query_list[i]);
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
