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
  int socketDesc;

  ClientSocket() {
    socketDesc = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDesc == -1) {
      throw std::runtime_error("Socket construction: " +
                               std::string(strerror(errno)));
    }
  }

 public:
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

  ~ClientSocket() { close(socketDesc); }
  ClientSocket(const ClientSocket &) = delete;
  ClientSocket(ClientSocket &&other) noexcept : socketDesc(other.socketDesc) {
    other.socketDesc = -1;
  }
  ClientSocket &operator=(const ClientSocket &) = delete;
  ClientSocket &operator=(ClientSocket &&other) noexcept {
    close(socketDesc);
    socketDesc = other.socketDesc;
    other.socketDesc = -1;
    return *this;
  }

  void sendData(const std::string data) {
    if (send(socketDesc, data.c_str(), data.length(), 0) == -1) {
      throw std::runtime_error("Socket data send: " +
                               std::string(strerror(errno)));
    }
  }

  std::string recvData(const int maxLen) {
    char buffer[maxLen];
    int length = recv(socketDesc, buffer, maxLen, 0);
    if (length < 0) {
      throw std::runtime_error("Socket data receive: " +
                               std::string(strerror(errno)));
    }
    std::cout << length << '\n';
    std::string ret(buffer, length);
    return ret;
  }
};

#endif /* CLIENT_SOCKET_HPP */