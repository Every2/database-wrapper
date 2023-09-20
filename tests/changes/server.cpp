#include <cstdint>
#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <WinSock2.h>
#include <cstdlib>
#include <cassert>
#include <array>


static void msg(std::string_view msg) {
    std::cerr << msg << '\n';
}

static void die(std::string_view msg) {
    int err {WSAGetLastError()};
    std::cerr << err << msg << '\n';
    std::abort();
}

static void fd_set_nb(SOCKET fd) {
    u_long mode {1};
    if (ioctlsocket(fd, FIONBIO, &mode) != 0) {
        die("ioctlsocket error");
    }
}

const size_t k_max_msg {4096};

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};

struct Conn {
    SOCKET fd {INVALID_SOCKET};
    uint32_t state {0};
    size_t rbuf_size {0};
    std::array<uint8_t, 4 + k_max_msg> rbuf;
    size_t wbuf_size {0};
    size_t wbuf_send {0};
    std::array<uint8_t, 4 + k_max_msg>  wbuf;
};

static void conn_put(std::vector<Conn *>& fd2conn, struct Conn *conn) {
    if (fd2conn.size() <= static_cast<size_t>(conn->fd)) {
        fd2conn.resize(conn->fd + 1);
    } 
    fd2conn.at(conn->fd) = conn;
}

static int32_t accept_new_conn(std::vector<Conn *>& fd2conn, SOCKET listenfd) {
    struct sockaddr_in client_addr {};
    int socketlen {sizeof(client_addr)};
    SOCKET connfd {accept(listenfd, reinterpret_cast<sockaddr *>(&client_addr), &socketlen)};
    if (connfd == INVALID_SOCKET) {
        msg("accept() error");
        return -1;
    }

    fd_set_nb(connfd);

    struct Conn *conn {new Conn};
    if (!conn) {
        closesocket(connfd);
        return -1;
    }

    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_send = 0;
    conn_put(fd2conn, conn);
    return 0;
}

static void state_req(Conn *conn);
static void state_res(Conn *conn);

static bool try_one_request(Conn *conn) {
    if (conn->rbuf_size < 4) {
        return false;
    }

    uint32_t len {0};
    memcpy(&len, &conn->rbuf.at(0), 4);
    if (len > k_max_msg) {
        msg("too long");
        conn->state = STATE_END;
        return false;
    }

    if (4 + len > conn->rbuf_size) {
        return false;
    }

    std::string client_msg(reinterpret_cast<char*>(&conn->rbuf.at(4)), len);
    std::cout << "client says: " << client_msg << '\n';

    memcpy(&conn->wbuf.at(0), &len, 4);
    memcpy(&conn->wbuf.at(4), &conn->rbuf.at(4), len);
    conn->wbuf_size = 4 + len;

    size_t remain {conn->rbuf_size - 4 - len};
    if (remain) {
        memmove(conn->rbuf.data(), &conn->rbuf.at(4 + len), remain);
    }
    conn->rbuf_size = remain;

    conn->state = STATE_RES;
    state_res(conn);

    return (conn->state == STATE_REQ);
} 


static bool try_fill_buffer(Conn *conn) {
    assert(conn->rbuf_size  < sizeof(conn->rbuf));

    int rv {recv(conn->fd, reinterpret_cast<char *>(&conn->rbuf[conn->rbuf_size]), sizeof(conn->rbuf) - conn->rbuf_size, 0)};
    if (rv < 0) {
        int error {WSAGetLastError()};
        if (error == WSAEWOULDBLOCK) {
            return false;
        }
        else {
            msg("recv() error");
            conn->state = STATE_END;
            return false;
        }
    }

    if (rv == 0) {
        if (conn->rbuf_size > 0) {
            msg("Unexpected EOF");
        }
        else {
            msg("EOF");
        }
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += static_cast<size_t>(rv);
    assert(conn->rbuf_size <= conn->rbuf.size());
    while(try_one_request(conn)) {};
    return (conn->state == STATE_REQ);
}

static void state_req(Conn *conn) {
    while(try_fill_buffer(conn)) {};
}

static bool try_flush_buffer(Conn *conn) {
    int rv {send(conn->fd, reinterpret_cast<char *>(&conn->wbuf[conn->wbuf_send]), conn->wbuf_size - conn->wbuf_send, 0)};
    if (rv < 0) {
        int error {WSAGetLastError()};
        if (error == WSAEWOULDBLOCK) {
            return false;
        }
        else {
            msg("send() error");
            conn->state = STATE_END;
            return false;
        }
    }
    conn->wbuf_send += rv;
    assert(conn->wbuf_send <= conn->wbuf_size);
    if (conn->wbuf_send == conn->wbuf_size) {
        conn->state = STATE_REQ;
        conn->wbuf_send = 0;
        conn->wbuf_size = 0;
        return false;
    }
    return true;
}

static void state_res(Conn *conn) {
    while(try_flush_buffer(conn)) {};
}

static void connection_io(Conn *conn) {
    if (conn->state == STATE_REQ) {
        state_req(conn);
    }
    else if (conn->state == STATE_RES) {
        state_res(conn);
    }
    else {
        assert(0);
    }
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

    int val {1};
    if  (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&val), sizeof(val) == SOCKET_ERROR)) {
        die("setsockopt()");
    }

    struct sockaddr_in addr {};
    addr.sin_family =  AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    int rv {bind(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr))};
    if (rv == SOCKET_ERROR) {
        die("bind()");
    }

    rv = listen(fd, SOMAXCONN);
    if (rv ==  SOCKET_ERROR) {
        die("listen()");
    }

    std::vector<Conn *> fd2conn;

    fd_set_nb(fd);

    std::vector<struct pollfd> poll_args;
    while (true) {
        fd_set readfds;
        fd_set writefds;
        fd_set exceptfds;

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_ZERO(&exceptfds);

        FD_SET(fd, &readfds);

        SOCKET maxfd = fd;

        for (Conn *conn: fd2conn) {
            if (!conn) {
                continue;
            }

            if (conn->state == STATE_REQ) {
                FD_SET(conn->fd, &readfds);
            }
            else if (conn->state == STATE_RES) {
                FD_SET(conn->fd, &writefds);
            }

            FD_SET(conn->fd, &exceptfds);

            if (conn->fd > maxfd) {
                maxfd = conn->fd;
            }
        }

        int rv {select(maxfd + 1, &readfds, &writefds, &exceptfds, NULL)};
        if (rv < 0) {
            die("select");
        }

        if (FD_ISSET(fd, &readfds)) {
            static_cast<void>(accept_new_conn(fd2conn, fd));
        }

        for (Conn *conn: fd2conn) {
            if (!conn) {
                continue;
            }

            if (FD_ISSET(conn->fd, &readfds) && conn->state == STATE_REQ) {
                state_req(conn);
            }

            if (FD_ISSET(conn->fd, &writefds) && conn->state == STATE_RES) {
                state_res(conn);
            }

            if (FD_ISSET(conn->fd, &exceptfds)) {
                conn->state = STATE_END;
            }

            if (conn->state == STATE_END) {
                fd2conn[conn->fd] = NULL;
                closesocket(conn->fd);
                delete [] conn;
            }
        }        
    }

    WSACleanup();
    return 0;
}