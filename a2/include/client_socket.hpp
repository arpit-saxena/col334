#ifndef CLIENT_SOCKET_HPP
#define CLIENT_SOCKET_HPP

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include "config.hpp"

class ClientSocket {
 public:
  enum Status { CLOSED, OPEN };

 private:
  std::string buffer;

 protected:
  int socketDesc;
  Status status;

  ClientSocket() {
    socketDesc = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDesc == -1) {
      throw std::runtime_error("Socket construction: " +
                               std::string(strerror(errno)));
    }
    status = OPEN;
  }

 public:
  ClientSocket(int desc) : socketDesc(desc), status(OPEN){};

  ClientSocket(const std::string serverAddr) : ClientSocket() {
    sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(serverAddr.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    if (connect(socketDesc, (sockaddr *)&server, sizeof(server))) {
      throw std::runtime_error("Socket connect to server: " +
                               std::string(strerror(errno)));
    }
  }

  ~ClientSocket() {
    // https://stackoverflow.com/a/8873013/5585431
    shutdown(socketDesc, SHUT_RDWR);
    char buffer[100];
    while (read(socketDesc, buffer, 100) > 0) {
    }
    close(socketDesc);
  }

  ClientSocket(const ClientSocket &) = delete;
  ClientSocket(ClientSocket &&other) noexcept
      : socketDesc(other.socketDesc), status(other.status) {
    other.socketDesc = -1;
    other.status = CLOSED;
  }
  ClientSocket &operator=(const ClientSocket &) = delete;
  ClientSocket &operator=(ClientSocket &&other) noexcept {
    close(socketDesc);
    socketDesc = other.socketDesc;
    status = other.status;
    other.socketDesc = -1;
    other.status = CLOSED;
    return *this;
  }

  Status operator()() const { return status; }

  void sendData(const std::string data);
  std::string recvSome(const int maxLen);
  std::string recv(const int len);
  std::string recvUntil(std::string matchStr);
};

#endif /* CLIENT_SOCKET_HPP */