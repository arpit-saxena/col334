#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include "client_socket.hpp"
#include "config.hpp"
#include "message.hpp"

void listenRecv(ClientSocket socket) {
  while (true) {
    try {
      socket.recvSome(READ_SIZE);
      if (!socket()) {
        std::cerr << "Receive socket closed! Exiting receive thread\n";
        return;
      }
      auto forward = ContentMessage::readFrom(socket, Message::FORWARD);
      std::cout << forward.getUsername() << ": " << forward.getContent()
                << std::endl;
      socket.sendData(
          ControlMessage{Message::RECEIVED, forward.getUsername()}.str());
    } catch (const ErrorMessage &e) {
      socket.sendData(e.str());
    }
  }
}

void listenSend(ClientSocket socket, std::string username) {
  std::string line;
  while (std::getline(std::cin, line)) {
    if (!socket()) {
      std::cerr << "Send socket closed! Exiting send thread\n";
      return;
    }

    std::istringstream ss{line};
    char at;
    ss >> at;
    if (at != '@') {
      std::cerr << "Expected @ symbol initially. Retry!\n";
      continue;
    }
    std::string toUser;
    ss >> toUser;
    ss.ignore(1);
    std::string content;
    std::getline(ss, content);

    socket.sendData(ContentMessage{Message::SEND, toUser, content}.str());
    try {
      socket.recvSome(READ_SIZE);
      try {
        auto res = ControlMessage::readFrom(socket, Message::SENT);
      } catch (const MessageTypeMismatch &m) {
        throw ErrorMessage::readFrom(socket);
      }
      std::cout << "Delivered successfully!" << std::endl;
    } catch (const ErrorMessage &e) {
      std::cerr << "Received error from server: " << e.getErrorStr()
                << std::endl;
    } catch (...) {
      std::cerr << "Send socket closed! Exiting send thread\n";
      return;
    }
  }
}

std::pair<std::string, std::string> getUsernameServer() {
  std::cout << "Enter Username: ";
  std::string username;
  std::cin >> username;
  std::string serverIP;
  std::cout << "Enter chat server address: ";
  std::cin >> serverIP;
  std::cin.ignore(1000, '\n');
  return {username, serverIP};
}

int main(int argc, char *argv[]) {
  while (true) {
    auto [username, serverIP] = getUsernameServer();
    try {
      ClientSocket sendSocket{serverIP}, recvSocket{serverIP};
      sendSocket.sendData(
          ControlMessage{Message::REGISTER, username, "TOSEND"}.str());

      sendSocket.recvSome(READ_SIZE);
      try {
        ControlMessage::readFrom(sendSocket, Message::REGISTERED);
      } catch (const MessageTypeMismatch &m) {
        throw ErrorMessage::readFrom(sendSocket);
      }
      std::cout << "Registered to send messages!" << std::endl;

      recvSocket.sendData(
          ControlMessage{Message::REGISTER, username, "TORECV"}.str());

      recvSocket.recvSome(READ_SIZE);
      try {
        ControlMessage::readFrom(recvSocket, Message::REGISTERED);
      } catch (const MessageTypeMismatch &m) {
        throw ErrorMessage::readFrom(recvSocket);
      }
      std::cout << "Registered to receive messages!" << std::endl;

      std::thread recvThread{listenRecv, std::move(recvSocket)};
      listenSend(std::move(sendSocket), username);
      recvThread.join();
      break;
    } catch (const std::runtime_error &e) {
      std::cerr << "Encountered error: " << e.what() << std::endl;
      std::cout << "Try again" << std::endl;
      continue;
    } catch (const MessageTypeMismatch &m) {
      std::cerr << "Found incorrect response from server" << std::endl;
      continue;
    } catch (const ErrorMessage &e) {
      std::cerr << "Server sent error: " << e.getErrorStr() << std::endl;
      continue;
    }
  }
}