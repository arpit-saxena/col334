#ifndef SERVER_SOCKET_HPP
#define SERVER_SOCKET_HPP

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstring>
#include <stdexcept>

#include "client_socket.hpp"

class ServerSocket : protected ClientSocket {
 public:
  ServerSocket(int port, int backlog) : ClientSocket() {
    // Allow reuse of socket address after it's closed
    // See https://stackoverflow.com/a/10651048/5585431
    int trueFlag = 1;
    setsockopt(socketDesc, SOL_SOCKET, SO_REUSEADDR, &trueFlag, sizeof(int));

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(socketDesc, (sockaddr *)&server, sizeof(server))) {
      throw std::runtime_error("ServerSocket bind: " +
                               std::string(strerror(errno)));
    }

    listen(socketDesc, backlog);
  }

  ClientSocket accept() {
    auto clientDesc = ::accept(socketDesc, NULL, NULL);
    if (clientDesc < 0) {
      throw std::runtime_error("ServerSocket accept: " +
                               std::string(strerror(errno)));
    }
    return ClientSocket{clientDesc};
  }
};

#endif /* SERVER_SOCKET_HPP */