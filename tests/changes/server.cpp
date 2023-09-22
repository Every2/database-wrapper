#include <cassert>
#include <cstdint>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <vector>
#include <map>
#include <iostream>
#include <string_view>
#include <array>

static void msg(std::string_view msg) {
    std::cerr << msg  << '\n';
}

static void die(std::string_view msg) {
    int err {WSAGetLastError()};
    std::cerr << err << msg << '\n';
    std::abort();
}

static void fd_set_nb(SOCKET fd) {
    u_long mode {1};
    if (ioctlsocket(fd, FIONBIO, &mode) == SOCKET_ERROR) {
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
    size_t wbuf_sent {0};
    std::array<uint8_t,  4 + k_max_msg> wbuf;
};

static void conn_put(std::vector<Conn *>& fd2conn, struct Conn *conn) {
    if (fd2conn.size() <= static_cast<size_t>(conn->fd)) {
        fd2conn.resize(conn->fd + 1);
    }
    fd2conn.at(conn->fd) = conn;
}

static int32_t accept_new_conn(std::vector<Conn *>& fd2conn, SOCKET listenSocket) {
    struct sockaddr_in client_addr {};
    int socklen {sizeof(client_addr)};
    SOCKET connSocket = accept(listenSocket, reinterpret_cast<sockaddr *>(&client_addr), &socklen);
    if (connSocket == INVALID_SOCKET) {
        msg("accept() error");
        return -1;  
    }

    fd_set_nb(connSocket);
    struct Conn *conn {new Conn};
    if (!conn) {
        closesocket(connSocket);
        return -1;
    }
    conn->fd = connSocket;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(fd2conn, conn);
    return 0;
}

static void state_req(Conn *conn);
static void state_res(Conn *conn);

const size_t k_max_args {1024};

static int32_t parse_req(const uint8_t *data, size_t len, std::vector<std::string>& out) {
    if (len < 4) {
        return -1;
    }
    uint32_t n {0};
    memcpy(&n, &data[0], 4);
    if (n > k_max_args) {
        return -1;
    }

    size_t pos {4};
    while (n--) {
        if (pos + 4 > len) {
            return -1;
        }
        uint32_t sz {0};
        memcpy(&sz, &data[pos], 4);
        if (pos + 4 + sz > len) {
            return -1;
        }
        out.push_back(std::string(reinterpret_cast<const char *>(&data[pos + 4]), sz));
        pos += 4 + sz;
    }

    if (pos != len) {
        return -1;  
    }
    return 0;
}

enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

static std::map<std::string, std::string> g_map;

static uint32_t do_get(const std::vector<std::string>& cmd, uint8_t *res, uint32_t *reslen) {
    if (!g_map.count(cmd.at(1))) {
        return RES_NX;
    }
    std::string& val {g_map.at(cmd.at(1))};
    assert(val.size() <= k_max_msg);
    memcpy(res, val.data(), val.size());
    *reslen = static_cast<uint32_t>(val.size());
    return RES_OK;
}

static uint32_t do_set(const std::vector<std::string>& cmd, uint8_t *res, uint32_t *reslen)
{
    static_cast<void>(res);
    static_cast<void>(reslen);
    g_map.at(cmd.at(1)) = cmd.at(2);
    return RES_OK;
}

static uint32_t do_del(const std::vector<std::string>& cmd, uint8_t *res, uint32_t *reslen)
{
    static_cast<void>(res);
    static_cast<void>(reslen);
    g_map.erase(cmd.at(1));
    return RES_OK;
}

static bool cmd_is(const std::string& word, std::string_view cmd) {
    return 0 == strcasecmp(word.c_str(), cmd.data());
}

static int32_t do_request(const uint8_t *req, uint32_t reqlen, uint32_t *rescode, uint8_t *res, uint32_t *reslen) {
    std::vector<std::string> cmd;
    if (0 != parse_req(req, reqlen, cmd)) {
        msg("bad req");
        return -1;
    }
    if (cmd.size() == 2 && cmd_is(cmd.at(0), "get")) {
        *rescode = do_get(cmd, res, reslen);
    } else if (cmd.size() == 3 && cmd_is(cmd.at(0), "set")) {
        *rescode = do_set(cmd, res, reslen);
    } else if (cmd.size() == 2 && cmd_is(cmd.at(0), "del")) {
        *rescode = do_del(cmd, res, reslen);
    } else {
        *rescode = RES_ERR;
        std::string msg {"Unknown cmd"};
        strcpy(reinterpret_cast<char *>(res), msg.data());
        *reslen = msg.length();
        return 0;
    }
    return 0;
}

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

    uint32_t rescode {0};
    uint32_t wlen {0};
    int32_t err {do_request(&conn->rbuf.at(4), len, &rescode, &conn->wbuf.at(4 + 4), &wlen)};
    if (err) {
        conn->state = STATE_END;
        return false;
    }
    wlen += 4;
    memcpy(&conn->wbuf.at(0), &wlen, 4);
    memcpy(&conn->wbuf.at(4), &rescode, 4);
    conn->wbuf_size = 4 + wlen;

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
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    int rv {0};
    do {
        size_t cap {sizeof(conn->rbuf) - conn->rbuf_size};
        rv = recv(conn->fd, reinterpret_cast<char *>(&conn->rbuf.at(conn->rbuf_size)), static_cast<int>(cap), 0);
    } while (rv < 0 && (WSAGetLastError() == WSAEINTR || WSAGetLastError() == WSAEWOULDBLOCK));
    if (rv < 0) {
        msg("recv() error");
        conn->state = STATE_END;
        return false;
    }
    if (rv == 0) {
        if (conn->rbuf_size > 0) {
            msg("unexpected EOF");
        } else {
            msg("EOF");
        }
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += static_cast<size_t>(rv);
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    while (try_one_request(conn)) {}
    return (conn->state == STATE_REQ);
}

static void state_req(Conn *conn) {
    while (try_fill_buffer(conn)) {}
}

static bool try_flush_buffer(Conn *conn) {
    int rv {0};
    do {
        size_t remain {conn->wbuf_size - conn->wbuf_sent};
        rv = send(conn->fd, reinterpret_cast<char*>(&conn->wbuf.at(conn->wbuf_sent)), static_cast<int>(remain), 0);
    } while (rv < 0 && (WSAGetLastError() == WSAEINTR || WSAGetLastError() == WSAEWOULDBLOCK));
    if (rv < 0) {
        msg("send() error");
        conn->state = STATE_END;
        return false;
    }
    conn->wbuf_sent += static_cast<size_t>(rv);
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
    while (try_flush_buffer(conn)) {}
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
        die("WSAStartup failed");
    }

    SOCKET fd {socket(AF_INET, SOCK_STREAM, 0)};
    if (fd == INVALID_SOCKET) {
        die("socket()");
    }

    int val {1};

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&val), sizeof(val)) == SOCKET_ERROR) {
        die("setsockopt()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    int rv {bind(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr))};
    if (rv) {
        die("bind()");
    }

    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

    std::vector<Conn *> fd2conn;

    fd_set_nb(fd);

    std::vector<SOCKET> poll_args;
    while (true) {
        poll_args.clear();
        poll_args.push_back(fd);  

        for (Conn *conn : fd2conn) {
            if (!conn) {
                continue;
            }
            poll_args.push_back(conn->fd);
        }

        fd_set read_fds, write_fds, except_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_ZERO(&except_fds);
        for (size_t i {0}; i < poll_args.size(); ++i) {
            if (i == 0) {
                FD_SET(poll_args.at(i), &read_fds);
            } else {
                if (fd2conn.at(poll_args.at(i))->state == STATE_REQ) {
                    FD_SET(poll_args.at(i), &read_fds);
                } else if (fd2conn.at(poll_args.at(i))->state == STATE_RES) {
                    FD_SET(poll_args.at(i), &write_fds);
                }
                FD_SET(poll_args.at(i), &except_fds);
            }
        }

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int numReady {select(0, &read_fds, &write_fds, &except_fds, &timeout)};
        if (numReady == SOCKET_ERROR) {
            die("select");
        }

        for (size_t i {0}; i < poll_args.size(); ++i) {
            if (i == 0) {
                if (FD_ISSET(fd, &read_fds)) {
                    static_cast<void>(accept_new_conn(fd2conn, fd));
                }
            } else {
                if (FD_ISSET(poll_args.at(i), &except_fds)) {
                    Conn *conn = fd2conn.at(poll_args.at(i));
                    conn->state = STATE_END;
                }
                if (FD_ISSET(poll_args.at(i), &read_fds)) {
                    Conn *conn = fd2conn.at(poll_args.at(i));
                    connection_io(conn);
                    if (conn->state == STATE_END) {

                        fd2conn.at(poll_args.at(i)) = nullptr;
                        closesocket(poll_args.at(i));
                        delete [] conn;
                    }
                }
                if (FD_ISSET(poll_args.at(i), &write_fds)) {
                    Conn *conn = fd2conn.at(poll_args.at(i));
                    connection_io(conn);
                    if (conn->state == STATE_END) {

                        fd2conn.at(poll_args.at(i)) = nullptr;
                        closesocket(poll_args.at(i));
                        delete []  conn;
                    }
                }
            }
        }
    }

    WSACleanup();
    return 0;
}