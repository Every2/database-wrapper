#include <vector>
#include <socket.hpp>
#include <cstdint>
#include <string>
#include <map>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <unistd.h>
#include "error.hpp"
#include "socket.hpp"
#include <memory>


class Server {

public:

    std::string accept_new_connection(std::vector <ConnectionInfo *> &fd_connection, int file_descriptor);
    std::string parser_request(const std::string& data, std::vector<std::string>& out);
    std::string set(const std::vector<std::string> &cmd, std::string &response);
    std::string get(const std::vector<std::string> &cmd, std::string &response); 
    std::string del(const std::vector<std::string> &cmd, std::string &response);
    std::string request(const std::string &request, std::string &response_code, std::string &response);
    bool try_one_request(ConnectionInfo *connection);
    bool try_fill_buffer(ConnectionInfo *connection);
    bool try_flush_buffer(ConnectionInfo *connection);
    void request_state(ConnectionInfo *connection);
    void response_state(ConnectionInfo *connection);
    void connection_io(ConnectionInfo *connection);
    void set_non_blocking(int file_descriptor);
    void put(std::vector <ConnectionInfo *> &fd_connection, std::shared_ptr<ConnectionInfo> conn);

private:
    
    std::map<std::string, std::string> server_map;

};
