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
    throw ErrorMessage(ErrorMessage::HEADER_INCOMPLETE);
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

int ErrorMessage::getErrorNum() const noexcept {
  switch (errorType) {
    case MALFORMED_USERNAME:
      return 100;
    case NO_USER_REG:
      return 101;
    case UNABLE_SEND:
      return 102;
    case HEADER_INCOMPLETE:
      return 103;
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