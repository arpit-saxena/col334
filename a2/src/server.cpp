#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "config.hpp"
#include "message.hpp"
#include "server_socket.hpp"

class UserMap {
  std::unordered_map<std::string, ClientSocket> clientMap;
  std::shared_mutex mapMutex;

 public:
  class UserNotFound : public std::exception {};

  // Attempts to add a user to the map. Returns true if the username didn't
  // exist and the insertion was successful, returns false otherwise. On
  // returning false, the map is unchanged
  bool addUser(std::string username, ClientSocket &&socket) {
    auto [it, inserted] = clientMap.try_emplace(username, std::move(socket));
    if (inserted) {
      it->second.addCleanupFunction(std::function<void(ClientSocket &)>{
          [this, username](ClientSocket &socket) { removeUser(username); }});
    }
    return inserted;
  }

  auto lockUnique() { return std::unique_lock{mapMutex}; }
  auto lockShared() { return std::shared_lock{mapMutex}; }

  ClientSocket &findUser(std::string username) {
    auto it = clientMap.find(username);
    if (it == clientMap.end()) throw UserNotFound{};
    return it->second;
  }

  void forEach(std::function<void(std::string, ClientSocket &)> func) {
    for (auto &[username, socket] : clientMap) {
      func(username, socket);
    }
  }

  bool userExists(std::string username) {
    return clientMap.find(username) != clientMap.end();
  }

  void removeUser(std::string username) {
    std::unique_lock lock{mapMutex};
    clientMap.erase(username);
  }
} userMap;

void forwardMessage(ClientSocket &receiverSocket, ContentMessage message) {
  receiverSocket.sendData(message.str());
  try {
    auto reply = ControlMessage::readFrom(receiverSocket, Message::RECEIVED);
    if (reply.getUsername() != message.getUsername()) {
      throw ErrorMessage{ErrorMessage::HEADER_INCOMPLETE};
    }
  } catch (MessageTypeMismatch m) {
    auto error = ErrorMessage::readFrom(receiverSocket);
    throw error;
  }
}

void forwardAll(ContentMessage message, std::string fromUser) {
  auto lock = userMap.lockUnique();
  userMap.forEach(
      [fromUser, message](std::string toUser, ClientSocket &receiverSocket) {
        if (fromUser == toUser) return;
        try {
          forwardMessage(receiverSocket, message);
        } catch (const ErrorMessage &e) {
          receiverSocket.sendData(e.str());
          if (e.getErrorType() == ErrorMessage::HEADER_INCOMPLETE) {
            receiverSocket.close();
          }
          throw ErrorMessage{ErrorMessage::UNABLE_SEND};
        }
      });
}

void receiveMessages(ClientSocket socket, std::string username) {
  while (true) {
    try {
      auto message = ContentMessage::readFrom(socket, Message::SEND);
      {
        auto lock = userMap.lockShared();
        if (!userMap.userExists(username)) {
          throw ErrorMessage{ErrorMessage::NO_USER_REG};
        }
      }
      try {
        if (message.getUsername() == "ALL") {
          auto forward =
              ContentMessage{Message::FORWARD, username, message.getContent()};
          forwardAll(forward, username);
          socket.sendData(
              ControlMessage{Message::SENT, message.getUsername()}.str());
          continue;
        }

        auto lock = userMap.lockShared();
        auto &receiverSocket = userMap.findUser(message.getUsername());
        lock.unlock();
        try {
          auto forward =
              ContentMessage{Message::FORWARD, username, message.getContent()};
          forwardMessage(receiverSocket, forward);
          socket.sendData(
              ControlMessage{Message::SENT, message.getUsername()}.str());
        } catch (const ErrorMessage &e) {
          receiverSocket.sendData(e.str());
          if (e.getErrorType() == ErrorMessage::HEADER_INCOMPLETE) {
            receiverSocket.close();
          }
          throw ErrorMessage{ErrorMessage::UNABLE_SEND};
        }
      } catch (const UserMap::UserNotFound &) {
        throw ErrorMessage{ErrorMessage::UNABLE_SEND};
      }
    } catch (ErrorMessage error) {
      socket.sendData(error.str());
      if (error.getErrorType() == ErrorMessage::HEADER_INCOMPLETE) {
        return;
      }
    } catch (MessageTypeMismatch m) {
      socket.sendData(ErrorMessage{ErrorMessage::HEADER_INCOMPLETE}.str());
      return;
    }
  }
}

void listenRegister(ClientSocket &&socket) {
  std::string username;
  try {
    auto message = ControlMessage::readFrom(socket, Message::REGISTER);
    username = message.getUsername();

    if (message.getAdditional() == "TORECV") {
      auto lock = userMap.lockUnique();
      bool inserted = userMap.addUser(username, std::move(socket));
      if (!inserted) {
        socket.sendData(ErrorMessage{ErrorMessage::MALFORMED_USERNAME}.str());
        return;
      }

      auto &recvSocket = userMap.findUser(username);
      recvSocket.sendData(
          ControlMessage{Message::REGISTERED, username, "TORECV"}.str());
      return;
    }
  } catch (ErrorMessage error) {
    socket.sendData(error.str());
    return;
  } catch (MessageTypeMismatch m) {
    socket.sendData(ErrorMessage{ErrorMessage::NO_USER_REG}.str());
    return;
  }

  socket.sendData(
      ControlMessage{Message::REGISTERED, username, "TOSEND"}.str());
  receiveMessages(std::move(socket), username);
}

int main(int argc, char *argv[]) {
  ServerSocket s{SERVER_PORT, 5};
  std::vector<std::thread> threads;
  while (true) {
    auto socket = s.accept();
    threads.emplace_back(listenRegister, std::move(socket));
  }
}