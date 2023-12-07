#include "tcp-server.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <list>
#include <string>

namespace TCP {

TcpServer::TcpServer(int protocol, int port, logging_foo logger,
                     int max_queue_length)
    : logger_(logger) {
  Logger(CServer, FConstructor, "Trying to create socket", Debug, logger_);
  listener_ = socket(AF_INET, SOCK_STREAM, protocol);
  if (listener_ < 0) {
    throw TcpException(TcpException::SocketCreation, errno);
  }

  sockaddr_in addr = {.sin_family = AF_INET,
                      .sin_port = htons(port),
                      .sin_addr = {htonl(INADDR_ANY)}};

  Logger(CServer, FConstructor, "Trying to bind to " + std::to_string(port),
         Info, logger_);
  if (bind(listener_, (sockaddr*)&addr, sizeof(addr)) < 0) {
    close(listener_);
    throw TcpException(TcpException::Binding, errno);
  }
  Logger(CServer, FConstructor, "Bound", Info, logger_);

  Logger(CServer, FConstructor, "Trying to listen", Info, logger_);
  if (listen(listener_, max_queue_length) < 0) {
    close(listener_);
    throw TcpException(TcpException::Listening, errno);
  }
  Logger(CServer, FConstructor, "Listening", Info, logger_);
}
TcpServer::TcpServer(int protocol, int port, int max_queue_length)
    : TcpServer(protocol, port, LoggerCap, max_queue_length) {}

TcpServer::~TcpServer() {
  close(listener_);
  while (!clients_.empty()) {
    close(clients_.cbegin()->dp_);
    clients_.erase(clients_.cbegin());
  }
  Logger(CServer, FDestructor,
         "Stopped listening, disconnected from all clients", Info, logger_);
}

std::list<TcpServer::Client>::iterator TcpServer::AcceptConnection() {
  Logger(CServer, FAcceptConnection, "Trying to accept connection", Info,
         logger_);
  int client = accept(listener_, NULL, NULL);

  if (client < 0) {
    throw TcpException(TcpException::Acceptance, errno);
  }

  clients_.push_front(Client(client));
  Logger(CServer, FAcceptConnection, LogSocket(client) + "Connection accepted",
         Info, logger_);
  return clients_.begin();
}

void TcpServer::CloseConnection(std::list<Client>::iterator client) {
  close(client->dp_);
  Logger(CServer, FCloseConnection,
         LogSocket(client->dp_) + "Connection closed", Info, logger_);

  clients_.erase(client);
}

bool TcpServer::IsAvailable(std::list<Client>::iterator client) {
  try {
    Logger(CServer, FIsAvailable,
           LogSocket(client->dp_) + "Trying to check if the data is available",
           Info, logger_);
    return TCP::IsAvailable(client->dp_);
  } catch (TcpException& tcp_exception) {
    if (tcp_exception.GetType() == TcpException::ConnectionBreak) {
      CloseConnection(client);
    }
    throw tcp_exception;
  }
}

}  // namespace TCP