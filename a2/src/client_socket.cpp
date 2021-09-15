#include "client_socket.hpp"

#include <stdexcept>

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
  if (buffer.size() != 0) {
    int retLen = std::min((int)buffer.length(), maxLen);
    std::string ret = buffer.substr(0, retLen);
    buffer = buffer.substr(retLen);
    return ret;
  }

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

// Read until first instance of matchStr and discard matchStr
std::string ClientSocket::recvUntil(std::string matchStr) {
  if (matchStr.size() == 0)
    throw std::invalid_argument("Can't read until empty string");

  std::string currReadStr;
  int idx = 0;

  while (true) {
    std::string message;
    if (buffer.size() > 0) {
      message = buffer;
      buffer = "";
    } else {
      message = recvSome(std::max((int)matchStr.length(), READ_SIZE));
    }

    if (message.length() == 0) {
      return currReadStr;
    }
    currReadStr += message;

    for (; idx + matchStr.length() <= currReadStr.length(); idx++) {
      if (currReadStr.substr(idx, matchStr.length()) == matchStr) {
        std::string ret = currReadStr.substr(0, idx);
        buffer = currReadStr.substr(idx + matchStr.length());
        return ret;
      }
    }
  }
}