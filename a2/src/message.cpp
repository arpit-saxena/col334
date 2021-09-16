#include "message.hpp"

#include <sstream>
#include <stdexcept>

std::string Message::getTypeStr(Type type) {
  switch (type) {
    case REGISTER:
      return "REGISTER";
    case REGISTERED:
      return "REGISTERED";
    case ERROR:
      return "ERROR";
    case SEND:
      return "SEND";
    case SENT:
      return "SENT";
    case FORWARD:
      return "FORWARD";
    case RECEIVED:
      return "RECEIVED";
  }
  throw std::invalid_argument("Invalid type");
}

std::string ControlMessage::str() const {
  std::ostringstream ss;
  ss << getTypeStr() << ' ';
  if (additional.length() > 0) ss << additional << ' ';
  ss << getUsername() << "\n\n";
  return ss.str();
}

ControlMessage ControlMessage::readFrom(ClientSocket &socket, Type type) {
  bool res = socket.expect(getTypeStr(type) + ' ');
  if (!res) {
    throw MessageTypeMismatch{};
  }

  std::string additional;
  switch (type) {
    case REGISTER:
    case REGISTERED:
      const std::string toSend{"TOSEND"};
      bool gotToSend = socket.expect(toSend + ' ');
      if (gotToSend) {
        additional = toSend;
        break;
      }
      const std::string toReceive{"TORECV"};
      bool gotToRecv = socket.expect(toReceive + ' ');
      if (gotToRecv) {
        additional = toReceive;
        break;
      }
      throw ErrorMessage(ErrorMessage::HEADER_INCOMPLETE);
  }

  const std::string username = socket.recvUntil("\n\n");
  for (auto c : username) {
    if (!isalnum(c)) throw ErrorMessage(ErrorMessage::MALFORMED_USERNAME);
  }

  return ControlMessage{type, username, additional};
}

std::string ContentMessage::str() const {
  std::ostringstream ss;
  ss << getTypeStr() << ' ' << getUsername() << '\n';
  ss << "Content-length: " << content.length() << "\n\n";
  ss << content;
  return ss.str();
}

ContentMessage ContentMessage::readFrom(ClientSocket &socket, Type type) {
  auto typeStr = Message::getTypeStr(type);
  auto found = socket.expect(typeStr + ' ');
  if (!found) throw MessageTypeMismatch{};
  auto username = socket.recvUntil("\n");
  found = socket.expect("Content-length: ");
  if (!found) throw ErrorMessage(ErrorMessage::HEADER_INCOMPLETE);
  auto lenStr = socket.recvUntil("\n\n");
  int len;
  try {
    len = std::stoi(lenStr);
  } catch (...) {
    throw ErrorMessage(ErrorMessage::HEADER_INCOMPLETE);
  }
  auto content = socket.recv(len);
  return ContentMessage{type, username, content};
}

int ErrorMessage::getErrorNum() const noexcept {
  switch (errorType) {
    case MALFORMED_USERNAME:
    case NO_USER_REG:
    case UNABLE_SEND:
    case HEADER_INCOMPLETE:
      return errorType;
  }
  return -1;
}

std::string ErrorMessage::getErrorStr() const noexcept {
  switch (errorType) {
    case MALFORMED_USERNAME:
      return "Malformed username";
    case NO_USER_REG:
      return "No user registered";
    case UNABLE_SEND:
      return "Unable to send";
    case HEADER_INCOMPLETE:
      return "Header incomplete";
  }
  return "Invalid type";
}

std::string ErrorMessage::str() const noexcept {
  std::ostringstream ss;
  ss << "ERROR " << getErrorNum() << ' ' << getErrorStr() << "\n\n";
  return ss.str();
}

ErrorMessage ErrorMessage::readFrom(ClientSocket &socket) {
  auto found = socket.expect("ERROR ");
  if (!found) throw MessageTypeMismatch{};
  auto errorNumStr = socket.recvUntil(" ");
  int errorNum;
  try {
    errorNum = std::stoi(errorNumStr);
  } catch (...) {
    throw ErrorMessage(ErrorMessage::HEADER_INCOMPLETE);
  }
  ErrorMessage err{(ErrorMessage::ErrorType)errorNum};
  if (err.getErrorNum() == -1) {
    throw ErrorMessage(ErrorMessage::HEADER_INCOMPLETE);
  }
  auto errStr = socket.recvUntil("\n\n");
  if (errStr != err.getErrorStr()) {
    throw ErrorMessage(ErrorMessage::HEADER_INCOMPLETE);
  }
  return err;
}