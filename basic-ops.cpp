#include "basic-ops.hpp"

namespace TCP {

TcpException::TcpException(ExceptionType type, int error)
    : type_(type), error_(error) {
  switch (type_) {
    case SocketCreation:
      s_what_ = "socket creation ";
      break;
    case Receiving:
      s_what_ = "receiving ";
      break;
    case ConnectionBreak:
      s_what_ = "connection break";
      break;
    case Sending:
      s_what_ = "sending ";
      break;
    case Binding:
      s_what_ = "binding ";
      break;
    case Listening:
      s_what_ = "listening ";
      break;
    case Acceptance:
      s_what_ = "acceptance ";
      break;
    case Connection:
      s_what_ = "connection ";
  }

  if (error_ != 0) {
    s_what_ += std::to_string(error_);
  }
}

const char* TcpException::what() const noexcept { return s_what_.c_str(); }
TcpException::ExceptionType TcpException::GetType() const noexcept {
  return type_;
}
int TcpException::GetErrno() const noexcept { return error_; }

std::string Receive(int socket) {
  std::string received;

  char num_buff[sizeof(int) + 1];
  // receive number of bytes to receive
  int b_recv = recv(socket, &num_buff, sizeof(int) + 1, 0);
  if (b_recv == 0) {
    throw TcpException(TcpException::ConnectionBreak);
  }
  if (b_recv < 0) {
    throw TcpException(TcpException::Receiving, errno);
  }
  int b_num;
  sscanf(num_buff, "%d", &b_num);

  // receive message
  char* buff = new char[b_num];
  b_recv = recv(socket, buff, b_num, 0);
  if (b_recv == 0) {
    delete[] buff;
    throw TcpException(TcpException::ConnectionBreak);
  }
  if (b_recv < 0) {
    delete[] buff;
    throw TcpException(TcpException::Receiving, errno);
  }

  for (int i = 0; i < b_recv; ++i) {
    received.push_back(buff[i]);
  }
  delete[] buff;
  return received;
}

void Send(int socket, const std::string& message) {
  // send number of bytes to send
  char num_of_bytes[sizeof(int) + 1];
  sprintf(num_of_bytes, "%d", message.size());
  int b_sent = send(socket, num_of_bytes, sizeof(int) + 1, 0);
  if (b_sent < 0) {
    throw TcpException(TcpException::Sending, errno);
  }

  // send message
  b_sent = send(socket, message.c_str(), message.size(), 0);
  if (b_sent < 0) {
    throw TcpException(TcpException::Sending, errno);
  }
}

}  // namespace TCP