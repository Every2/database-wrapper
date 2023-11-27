#include "../include/server.hpp"


std::string Server::accept_new_connection(
    std::vector <ConnectionInfo *> &fd_connection, 
    int file_descriptor) 
{
    struct sockaddr_in client_addr = {};
    socklen_t socklen {sizeof(client_addr)};
    int connect_file_descriptor {accept(file_descriptor, reinterpret_cast<sockaddr *>(&client_addr), &socklen)};
    
    if (connect_file_descriptor < 0) {
        return "accept() error";
    }

    set_non_blocking(connect_file_descriptor);

    std::shared_ptr<ConnectionInfo> shared_ptr_connection {std::make_shared<ConnectionInfo>()};

    if (!shared_ptr_connection) {
        close(connect_file_descriptor);
        return "alocation error";
    }

    shared_ptr_connection->file_descriptor = connect_file_descriptor;
    shared_ptr_connection->state = ConnectionState::request_state;
    put(fd_connection, shared_ptr_connection);
}