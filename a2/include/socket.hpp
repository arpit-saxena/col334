#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include "config.hpp"

class Socket {
  int socketDesc;

  Socket() {
    socketDesc = socket(AF_INET, SOCK_STREAM, 0);
    if (socketDesc == -1) {
      throw std::runtime_error("Socket construction: " +
                               std::string(strerror(errno)));
    }
  }

 public:
  Socket(const std::string serverAddr) : Socket() {
    sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(serverAddr.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    if (connect(socketDesc, (sockaddr *)&server, sizeof(server))) {
      throw std::runtime_error("Socket connect to server: " +
                               std::string(strerror(errno)));
    }
  }

  ~Socket() { close(socketDesc); }
  Socket(const Socket &) = delete;
  Socket(Socket &&other) noexcept : socketDesc(other.socketDesc) {
    other.socketDesc = -1;
  }
  Socket &operator=(const Socket &) = delete;
  Socket &operator=(Socket &&other) noexcept {
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

#endif /* SOCKET_HPP */