#include "client_socket.hpp"

#include <cctype>
#include <stdexcept>

void ClientSocket::close() {
  if (status == CLOSED) return;

  // https://stackoverflow.com/a/8873013/5585431
  shutdown(socketDesc, SHUT_RDWR);
  char buffer[100];
  while (read(socketDesc, buffer, 100) > 0) {
  }
  ::close(socketDesc);

  for (auto func : cleanupFunctions) {
    try {
      func(*this);
    } catch (...) {
    }
  }

  status = CLOSED;
}

void ClientSocket::sendData(const std::string data) {
  if (send(socketDesc, data.c_str(), data.length(), MSG_NOSIGNAL) == -1) {
    if (errno == EPIPE) {
      close();
      return;
    }
    throw std::runtime_error("Socket data send: " +
                             std::string(strerror(errno)));
  }
}

void ClientSocket::recvSome(const int maxLen) {
  if (buffer.size() != 0) {
    std::clog << "recvSome called with non-empty buffer! Emptying buffer\n";
    buffer.clear();
  }

  char charBuf[maxLen];
  auto length = ::recv(socketDesc, charBuf, maxLen, 0);
  if (length < 0) {
    throw std::runtime_error("Socket data receive: " +
                             std::string(strerror(errno)));
  }
  if (length == 0) status = CLOSED;
  buffer = std::string{charBuf, (size_t)length};
}

// Read from buffer until first instance of matchStr or end of buffer
// If some part was matched returns true, otherwise returns false
// The returned data is removed from the buffer.
std::pair<std::string, bool> ClientSocket::recvUntil(
    const std::string matchStr) {
  if (matchStr.size() == 0)
    throw std::invalid_argument("Can't read until empty string");

  for (int idx = 0; idx + matchStr.length() <= buffer.length(); idx++) {
    if (buffer.substr(idx, matchStr.length()) == matchStr) {
      std::string ret = buffer.substr(0, idx);
      buffer = buffer.substr(idx + matchStr.length());
      return {ret, true};
    }
  }

  auto ret = buffer;
  buffer.clear();
  return {ret, false};
}

// Reads string str from the buffer and discards it. If the string does
// not match returns false, buffer is left unchanged
bool ClientSocket::expect(const std::string str) {
  if (buffer.size() < str.size()) return false;
  if (buffer.substr(0, str.size()) == str) {
    buffer = buffer.substr(str.size());
    return true;
  }
  return false;
}

// Empties the buffer and returns the contents of it;
std::string ClientSocket::recvAll() {
  std::string ret = buffer;
  buffer.clear();
  return ret;
}