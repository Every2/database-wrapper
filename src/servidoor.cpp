#include <winsock2.h>
#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <cassert>
#include <cstring>
#include <memory>

using namespace std::string_literals;

const size_t k_max_msg = 4096; 

static void state_req(Conn *conn);
static void state_res(Conn *conn);

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};

struct Conn{
    SOCKET fd = -1;
    uint32_t state = 0;
    size_t rbuf_size = 0;
    std::array<char, 4 + k_max_msg> rbuf{};
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    std::array<char, 4 + k_max_msg> wbuf{};
};

static void msg(const std::string msg) {
    std::cout << msg;
}

static void die(const std::string msg) {
    std::cout << msg << WSAGetLastError() << '\n';
    abort();
}

static void fd_set_nb(SOCKET fd) {
    u_long mode = 1;
    if (ioctlsocket(fd, FIONBIO, &mode) != 0) {
        std::cout  << "Failed to set socket to non-blocking mode"s << '\n';
        return;
    }
}



static int32_t accept_new_conn(std::vector<std::unique_ptr<Conn>>& fd2conn, SOCKET fd) {
    struct sockaddr_in client_addr =  {};
    int socklen = sizeof(client_addr);
    SOCKET connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if  (connfd == INVALID_SOCKET) {
        std::cout << "Failed to accept new connection" << '\n';
        return -1;
    }

    fd_set_nb(connfd);
    std::unique_ptr<Conn> newConn = std::make_unique<Conn>();
    if (!newConn) {
        closesocket(connfd);
        return -1;
    }
    newConn->fd = connfd;
    newConn->state = STATE_REQ;
    newConn->rbuf_size = 0;
    newConn->wbuf_size = 0;
    newConn->wbuf_sent = 0;

   fd2conn.push_back(std::move(newConn));
   
   

   return 0;
}

static bool try_flush_buffer(Conn *conn) {
    int rv = 0;
    do {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = send(conn->fd, &conn->wbuf[conn->wbuf_sent], remain, 0);
    } while (rv < 0 && WSAGetLastError() == WSAEINTR);
    if (rv < 0 && WSAGetLastError() == EAGAIN) {
        return false;
    }

    if (rv < 0) {
        msg("send() error");
        conn->state = STATE_END;
        return false;
    }
    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);
    if (conn->wbuf_sent == conn->wbuf_size) {
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }

    return true;
}

static void state_res(Conn *conn) {
    while(try_flush_buffer(conn)) {}
}

static bool try_one_request(Conn *conn) {
    if (conn->rbuf_size < 4) {
        return false;
    }

    uint32_t len = 0;
    std::memcpy(&len, &conn->rbuf[0], 4);
    if (len > k_max_msg) {
        msg("too long");
        conn->state = STATE_END;
        return false;
    }

    if (4 + len > conn->rbuf_size) {
        return false;
    }

    std::cout << "Client says: " << len << &conn->rbuf[4];

    std::memcpy(&conn->wbuf[0],  &len, 4);
    std::memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
    conn->wbuf_size = 4 + len;

    size_t remain = conn->rbuf_size - 4 - len;
    if (remain) {
        std::memmove(&conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain;

    conn->state  = STATE_REQ;
    state_res(conn);

    return (conn->state == STATE_REQ);
}

static bool try_fill_buffer(Conn *conn) {
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    int rv = 0;
    do {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = recv(conn->fd, conn->rbuf.data() + conn->rbuf_size, static_cast<int>(cap), 0);
    } while (rv < 9 && (WSAGetLastError() == WSAEINTR));

    if (rv == 0) {
        if (conn->rbuf_size > 0) {
            std::cout << "Unexpected EOF" << '\n';
        } else {
            std::cout << "EOF" << '\n';
        }
        conn->state = STATE_END;
        return false;
    }

    if (rv < 0) {
        std::cout << "recv() error" << '\n';
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += static_cast<size_t>(rv);
    assert(conn->rbuf_size <= conn->rbuf.size());

    while(try_one_request(conn)) {}
    return (conn->state == STATE_REQ);
}

static void state_req(Conn *conn) {
    while(try_fill_buffer(conn)) {}
}


static void connection_io(Conn *conn) {
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

    std::vector<std::unique_ptr<Conn, std::default_delete<Conn>>> uniquePtrVector;
    std::vector<Conn *> fd2conn;
    
    fd_set_nb(fd);

    std::vector<struct pollfd> poll_args{};
    while (true) {
        poll_args.clear();

        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);

        for (size_t i = 0; i < fd2conn.size(); ++i) {
            Conn* conn = fd2conn[i];
            if (!conn) {
                continue;
            }       
            struct pollfd pfd = {};
            pfd.fd = static_cast<SOCKET>(i);
            pfd.events = (conn->state == STATE_REQ) ? POLLIN  : POLLOUT;
            pfd.events = pfd.events | POLLERR;
            poll_args.push_back(pfd);
        }

        int rv = WSAPoll(poll_args.data(), poll_args.size(), 1000);
        if (rv < 0) {
            die("pool");
        }

        for (size_t i = 1; i < poll_args.size(); ++i) {
            if (poll_args[i].revents) {
                Conn *conn = fd2conn[poll_args[i].fd];
                connection_io(conn);
                if (conn->state == STATE_END) {
                    fd2conn[conn->fd] = nullptr;
                    closesocket(conn->fd);
                }
            }
        }

        
        if (poll_args[0].revents) {
            (void)accept_new_conn(uniquePtrVector, fd);
        }
    }

    closesocket(fd);
    WSACleanup();

    return 0;
}