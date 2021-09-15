#include "client_socket.hpp"

void ClientSocket::sendData(const std::string data) {
  if (send(socketDesc, data.c_str(), data.length(), MSG_NOSIGNAL) == -1) {
    if (errno == EPIPE) {
      status = CLOSED;
      return;
    }
    throw std::runtime_error("Socket data send: " +
                             std::string(strerror(errno)));
  }
}

std::string ClientSocket::recvSome(const int maxLen) {
  char charBuf[maxLen];
  int length = ::recv(socketDesc, charBuf, maxLen, 0);
  if (length < 0) {
    throw std::runtime_error("Socket data receive: " +
                             std::string(strerror(errno)));
  }
  if (length == 0) status = CLOSED;
  std::string ret(charBuf, length);
  return ret;
}

std::string ClientSocket::recv(const int len) {
  std::string ret{};
  while (ret.length() < len) {
    ret += recvSome(len - ret.length());
  }
  return ret;
}