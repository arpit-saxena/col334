#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>

#include "client_socket.hpp"

class Message {
 public:
  enum Type { REGISTER, REGISTERED, ERROR, SEND, SENT, FORWARD, RECEIVED };

 private:
  Type type;

 protected:
  Message(Type _type) : type(_type){};

 public:
  Type getType() const { return type; }
  static std::string getTypeStr(Type type);
  std::string getTypeStr() const { return getTypeStr(type); };
};

// Objects of this class are thrown when message type is not what was expected
// when parsing data from a socket
class MessageTypeMismatch {};

class ControlMessage : public Message {
  std::string username;
  std::string additional;

 public:
  ControlMessage(Type _type, std::string _username,
                 std::string _additional = "")
      : Message(_type), username(_username), additional(_additional){};

  std::string getUsername() const { return username; };
  std::string getAdditional() const { return additional; };
  std::string str() const;
  static ControlMessage readFrom(ClientSocket &socket, Type type);
};

class ContentMessage : public Message {
  std::string username;
  std::string content;

 public:
  ContentMessage(Type _type, std::string _username, std::string _content)
      : Message(_type), username(_username), content(_content){};

  std::string getUsername() const { return username; };
  std::string getContent() const { return content; };
  std::string str() const;
  static ContentMessage readFrom(ClientSocket &socket, Type type);
};

class ErrorMessage : public Message {
 public:
  enum ErrorType {
    MALFORMED_USERNAME = 100,
    NO_USER_REG = 101,
    UNABLE_SEND = 102,
    HEADER_INCOMPLETE = 103
  };

 private:
  ErrorType errorType;
  int getErrorNum() const noexcept;

 public:
  ErrorMessage(ErrorType _errorType) : Message(ERROR), errorType(_errorType){};
  std::string str() const noexcept;
  ErrorType getErrorType() const noexcept { return errorType; };
  std::string getErrorStr() const noexcept;
  static ErrorMessage readFrom(ClientSocket &socket);
};

#endif /* MESSAGE_HPP */