#include <winsock2.h>
#include <iostream>
#include <string>
#include <cassert>
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

static int32_t query(int fd, std::string text) {
    uint32_t len = (uint32_t)text.size();
    if (len > k_max_msg) 
    {
        return -1;
    }

    std::vector<char> wbuf(4 + k_max_msg);
    std::memcpy(wbuf.data(), &len, 4);
    std::memcpy(wbuf.data(), text.data(), len);
    if (int32_t err = write_all(fd, wbuf, 4 + len)) {
        return err;
    }

    std::vector<char> rbuf(4 + k_max_msg + 1);
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err)
    {
        if (errno == 0) {
            msg("EOF");
        }
        else {
            msg("read() error");
        }

        return err;
    }
    std::memcpy(&len, rbuf.data(), 4);
    if (len > k_max_msg)
    {
        msg("too long");
        return err;
    }

    rbuf[4 + len] = '\0';
    std::string serverMsg(rbuf.begin(), rbuf.end());
    std::cout << "Server says: " << serverMsg << '\n';
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

    int32_t err = query(fd, "hello1"s);
    if (err) {
        goto L_DONE;
    }

    err = query(fd, "hello2"s);
    if (err) {
        goto L_DONE;
    }

    err = query(fd, "hello3"s);
    if (err) {
        goto L_DONE;
    }

    L_DONE:
        closesocket(fd);
        return 0;
    
    return 0;
}