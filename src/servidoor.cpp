#include <algorithm>
#include <iostream>
#include <winsock2.h>
#include <string>
#include <array>
#include <vector>
#include <cassert>
#include <cstring>
#include <memory>

using namespace std::string_literals;

static void msg(const std::string msg) {
    std::cerr << msg << '\n';
}

static void die(const std::string msg) {
    std::cerr << msg << '[' << WSAGetLastError() << ']' << '\n';
    abort();
}

static void fd_set_nb(SOCKET fd) {
    u_long mode = 1;
    if (ioctlsocket(fd, FIONBIO, &mode) == SOCKET_ERROR) {
        die("Failed to set socket to non-blocking mode"s);
        return;
    }
}

const size_t k_max_msg = 4096; 

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};

struct Conn{
    SOCKET fd = INVALID_SOCKET;
    uint32_t state = 0;
    std::vector<char> rbuf{};
    std::vector<char> wbuf{};
    size_t wbuf_sent = 0;
};

static void conn_put(std::vector<std::unique_ptr<Conn>>& fd2conn, Conn* conn) {
    if (fd2conn.size() <= static_cast<size_t>(conn->fd)) {
        fd2conn.resize(conn->fd + 1);
    }
    fd2conn[conn->fd] = std::unique_ptr<Conn>(conn);
}

static std::unique_ptr<Conn> accept_new_conn(SOCKET listenSocket) {
    struct sockaddr_in client_addr = {};
    int client_len = sizeof(client_addr);
    SOCKET connfd = accept(listenSocket, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
    if (connfd == INVALID_SOCKET) {
        msg("accept() error");
        return nullptr;
    }

    fd_set_nb(connfd);

    auto conn = std::make_unique<Conn>();
    conn->fd = connfd;
    conn->state = STATE_REQ;

    return conn;
}

static void state_req(Conn *conn);
static void state_res(Conn *conn);

static bool try_one_request(Conn *conn) {
    if (conn->rbuf.size() < 4) {
        return false;
    }

    uint32_t len = 0;
    std::memcpy(&len, &conn->rbuf[0], 4);
    if (len > k_max_msg) {
        msg("too long");
        conn->state = STATE_END;
        return false;
    }

    if (4 + len > conn->rbuf.size()) {
        return false;
    }

    std::cout << "Client says: " << std::string(reinterpret_cast<char*>(&conn->rbuf[4]), len)  << '\n';

    conn->wbuf.resize(4 + len);
    std::memcpy(&conn->wbuf[0], &len, 4);
    std::memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
    
    conn->wbuf.erase(conn->rbuf.begin(), conn->rbuf.begin() + 4 + len);
    
    conn->state = STATE_RES;
    state_res(conn);

    return conn->state == STATE_REQ;
}

static bool try_fill_buffer(Conn *conn) {
    std::vector<char> buf(k_max_msg);

    int rv = recv(conn->fd, buf.data(), buf.size(), 0);
    if (rv == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) {
            msg("recv() error"s);
            conn->state = STATE_END;
            return false;
        }
    } else if (rv == 0) {
        if (!conn->rbuf.empty()) {
            msg("Unexpected EOF()");
        } else {
            msg("EOF");
        }
        conn->state = STATE_END;
        return false;
    } else {
        conn->rbuf.insert(conn->rbuf.end(), buf.begin(),  buf.begin() + rv);

        while(try_one_request(conn)) {}
    }

    return conn->state == STATE_REQ;
}

static void state_req(Conn *conn) {
    while(try_fill_buffer(conn)) {}
}

static bool try_flush_buffer(Conn *conn) {
    int rv{send(conn->fd, conn->wbuf.data() + conn->wbuf_sent, static_cast<int>(conn->wbuf.size() - conn->wbuf_sent), 0)};
    if (rv == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) {
            msg("send() error");
            conn->state = STATE_END;
            return false;
        }
    } else {
        conn->wbuf_sent += rv;
        assert(conn->wbuf_sent <= conn->wbuf.size());
        if (conn->wbuf_sent == conn->wbuf.size()) {
            conn->state = STATE_REQ;
            conn->wbuf_sent = 0;
            conn->wbuf.clear();
            return false;
        }
    }

    return true;
}

static void state_res(Conn *conn) {
    while(try_flush_buffer(conn)) {}
}

static void connection_io(Conn* conn) {
    if (conn->state == STATE_REQ) {
        state_req(conn);
    } else if (conn->state == STATE_RES) {
        state_res(conn);
    } else {
        assert(0);
    }
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
    std::string convertedVal {std::to_string(val)};
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, convertedVal.data(), sizeof(val)) == SOCKET_ERROR) {
        die("setsockopt()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        die("bind()"s);
    }

    if (listen(fd, SOMAXCONN) == SOCKET_ERROR) {
        die("listen()"s);
    }

    std::vector<std::unique_ptr<Conn>> fd2conn;
    
    fd_set_nb(fd);

    while (true) {
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(fd, &readFds);

        SOCKET maxfd{fd};
        for (const auto& conn: fd2conn) {
            if (conn) {
                FD_SET(conn->fd, &readFds);
                maxfd = std::max(maxfd, conn->fd);
            }
        }

        int rv = select(static_cast<int>(maxfd + 1), &readFds, nullptr, nullptr, nullptr);
        if (rv == SOCKET_ERROR) {
            die("select");
        }

        if (FD_ISSET(fd, &readFds)) {
            auto conn = accept_new_conn(fd);
            if (conn) {
                conn_put(fd2conn, conn.release());
            }
        }

        for (const auto& conn: fd2conn) {
            if (conn && FD_ISSET(conn->fd, &readFds)) {
                connection_io(conn.get());
                if (conn->state == STATE_END) {
                    closesocket(conn->fd);
                }
            }
        }
    }

    WSACleanup();

    return 0;
}