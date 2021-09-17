#ifndef CLIENT_SOCKET_HPP
#define CLIENT_SOCKET_HPP

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.hpp"

class ClientSocket {
 public:
  enum Status { CLOSED, OPEN };

 private:
  std::string buffer;
  std::vector<std::function<void(ClientSocket &)>> cleanupFunctions;

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

  void addCleanupFunction(std::function<void(ClientSocket &)> func) {
    cleanupFunctions.push_back(func);
  };

  void close();

  ~ClientSocket() { close(); }

  ClientSocket(const ClientSocket &) = delete;
  ClientSocket(ClientSocket &&other) noexcept
      : socketDesc(std::move(other.socketDesc)),
        status(std::move(other.status)),
        cleanupFunctions(std::move(other.cleanupFunctions)) {
    other.socketDesc = -1;
    other.status = CLOSED;
  }

  ClientSocket &operator=(const ClientSocket &) = delete;
  ClientSocket &operator=(ClientSocket &&other) noexcept {
    close();
    socketDesc = other.socketDesc;
    status = other.status;
    cleanupFunctions = other.cleanupFunctions;
    other.socketDesc = -1;
    other.status = CLOSED;
    other.cleanupFunctions.clear();
    return *this;
  }

  Status operator()() const { return status; }

  void sendData(const std::string data);
  std::string recvSome(const int maxLen);
  std::string recv(const int len);
  std::string recvUntil(std::string matchStr);
  bool expect(std::string str);
};

#endif /* CLIENT_SOCKET_HPP */