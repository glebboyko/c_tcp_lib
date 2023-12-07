#include "tcp-client.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <list>
#include <string>

namespace TCP {

TcpClient::TcpClient(int protocol, int port, const char* server_addr,
                     logging_foo logger)
    : logger_(logger) {
  Logger(CClient, FConstructor, "Trying to create socket", Debug, logger_);
  connection_ = socket(AF_INET, SOCK_STREAM, 0);
  if (connection_ < 0) {
    throw TcpException(TcpException::SocketCreation, errno);
  }
  Logger(CClient, FConstructor, "Socket created", Debug, logger_);

  Logger(CClient, FConstructor,
         "Trying to set connection to " + std::string(server_addr) + ":" +
             std::to_string(port),
         Info, logger_);
  sockaddr_in addr = {.sin_family = AF_INET,
                      .sin_port = htons(port),
                      .sin_addr = {inet_addr(server_addr)}};
  if (connect(connection_, (sockaddr*)&addr, sizeof(addr)) < 0) {
    close(connection_);
    throw TcpException(TcpException::Connection, errno);
  }
  Logger(CClient, FConstructor, "Connection set", Info, logger_);
}

TcpClient::~TcpClient() {
  close(connection_);
  Logger(CClient, FDestructor, "Disconnected", Info, logger_);
}

bool TcpClient::IsAvailable() {
  Logger(CClient, FIsAvailable,
         LogSocket(connection_) + "Trying to check if the data is available",
         Info, logger_);
  return TCP::IsAvailable(connection_);
}

}  // namespace TCP