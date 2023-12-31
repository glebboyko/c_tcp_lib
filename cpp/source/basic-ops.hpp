#pragma once

#include <sys/socket.h>

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

#include "tcp-supply.hpp"

namespace TCP {

void LoggerCap(const std::string& l_module, const std::string& l_action,
               const std::string& l_event, int priority);

enum LModule { CServer, CClient, CExternFoo, CException };
enum LAction {
  FConstructor,
  FMoveConstr,
  FFromServerConstr,
  FDestructor,
  FAcceptConnection,
  FCloseConnection,
  FCloseListener,
  FIsAvailable,
  FReceive,
  FSend,
  FException
};

void Logger(LModule, LAction, const std::string&, int priority,
            logging_foo logger, void* module_address = nullptr);

std::string LogSocket(int socket);

class TcpException : public std::exception {
 public:
  enum ExceptionType {
    SocketCreation,
    Receiving,
    ConnectionBreak,
    Sending,

    Binding,
    Listening,
    Acceptance,

    Connection,

    Setting,

    Multithreading
  };

  TcpException(ExceptionType type, int error = 0, bool message_leak = false);
  TcpException(ExceptionType type, logging_foo logger, int error = 0,
               bool message_leak = false);

  const char* what() const noexcept override;
  ExceptionType GetType() const noexcept;
  int GetErrno() const noexcept;

 private:
  ExceptionType type_;
  int error_;
  std::string s_what_;
};

bool IsAvailable(int socket, logging_foo logger = LoggerCap);

void ToArgs(std::stringstream& stream);
template <typename Head, typename... Tail>
void ToArgs(std::stringstream& stream, Head& head, Tail&... tail) {
  stream >> head;
  ToArgs(stream, tail...);
}

const int kIntMaxDigitNum = 10;

template <typename... Args>
void Receive(int socket, logging_foo logger, Args&... args) {
  auto log_socket = LogSocket(socket);

  Logger(CExternFoo, FReceive, log_socket + "Trying to receive data", Info,
         logger);

  Logger(CExternFoo, FReceive, log_socket + "Trying to receive message length",
         Debug, logger);
  char num_buff[kIntMaxDigitNum + 1];
  // receive number of bytes to receive
  int b_recv = recv(socket, &num_buff, kIntMaxDigitNum + 1, 0);
  if (b_recv == 0) {
    throw TcpException(TcpException::ConnectionBreak, logger);
  }
  if (b_recv < 0) {
    throw TcpException(TcpException::Receiving, logger, errno);
  }
  if (b_recv < kIntMaxDigitNum + 1) {
    throw TcpException(TcpException::Receiving, logger, 0, true);
  }

  int b_num;
  sscanf(num_buff, "%d", &b_num);
  Logger(CExternFoo, FReceive,
         log_socket + "Message length is " + std::to_string(b_num) + " bytes",
         Debug, logger);

  // receive message
  Logger(CExternFoo, FReceive, log_socket + "Trying to receive main message",
         Debug, logger);
  char* buff = new char[b_num];
  b_recv = recv(socket, buff, b_num, 0);
  if (b_recv == 0) {
    delete[] buff;
    throw TcpException(TcpException::ConnectionBreak, logger);
  }
  if (b_recv < 0) {
    delete[] buff;
    throw TcpException(TcpException::Receiving, logger, errno);
  }
  if (b_recv < b_num) {
    throw TcpException(TcpException::Receiving, logger, 0, true);
  }
  Logger(CExternFoo, FReceive,
         log_socket + "Received " + std::to_string(b_recv) + " bytes", Debug,
         logger);

  Logger(CExternFoo, FReceive,
         log_socket + "Trying to extract arguments from message", Debug,
         logger);
  std::stringstream stream;
  stream << buff;

  delete[] buff;

  ToArgs(stream, args...);

  Logger(CExternFoo, FReceive, log_socket + "Message received", Info, logger);
}

void FromArgs(std::string& output);
template <typename Head, typename... Tail>
void FromArgs(std::string& output, const Head& head, const Tail&... tail) {
  std::stringstream stream;
  stream << head;
  std::string str_head;
  stream >> str_head;

  if (!output.empty()) {
    output.push_back(' ');
  }
  output += str_head;

  FromArgs(output, tail...);
}

template <typename... Args>
void Send(int socket, logging_foo logger, const Args&... args) {
  auto log_socket = LogSocket(socket);
  Logger(CExternFoo, FSend, log_socket + "Trying to send data", Info, logger);

  Logger(CExternFoo, FSend, log_socket + "Putting arguments to string", Debug,
         logger);
  std::string input;
  FromArgs(input, args...);

  // send number of bytes to send
  Logger(CExternFoo, FSend, log_socket + "Trying to send message length", Debug,
         logger);
  char num_buff[kIntMaxDigitNum + 1];
  for (int i = 0; i < kIntMaxDigitNum + 1; ++i) {
    num_buff[i] = '\0';
  }
  sprintf(num_buff, "%d", input.size() + 1);
  int b_sent = send(socket, num_buff, kIntMaxDigitNum + 1, 0);
  if (b_sent < 0) {
    if (errno == ECONNRESET) {
      throw TcpException(TcpException::ConnectionBreak, logger);
    }
    throw TcpException(TcpException::Sending, logger, errno);
  }
  if (b_sent < kIntMaxDigitNum + 1) {
    throw TcpException(TcpException::Sending, logger, 0, true);
  }
  Logger(CExternFoo, FSend, log_socket + "Message length sent", Debug, logger);

  Logger(CExternFoo, FSend, log_socket + "Trying to send main message", Debug,
         logger);
  // send message
  b_sent = send(socket, input.c_str(), input.size() + 1, 0);
  if (b_sent < 0) {
    if (errno == ECONNRESET) {
      throw TcpException(TcpException::ConnectionBreak, logger);
    }
    throw TcpException(TcpException::Sending, logger, errno);
  }
  if (b_sent < input.size() + 1) {
    throw TcpException(TcpException::Sending, logger, 0, true);
  }
  Logger(CExternFoo, FSend, log_socket + "Main message sent", Debug, logger);

  Logger(CExternFoo, FSend, log_socket + "Message sent", Info, logger);
}

}  // namespace TCP