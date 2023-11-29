#pragma once

#include <list>
#include <string>

class TcpServer {
 public:
  struct Client {
   private:
    Client() = default;
    Client(int descriptor) : dp_(descriptor) {}

    int dp_;
    friend TcpServer;
  };

  TcpServer(unsigned char domain_space, int protocol, int port);
  ~TcpServer();

  auto AcceptConnection();
  void CloseConnection(std::list<Client>::iterator client);

  std::string Receive(std::list<Client>::iterator client);
  bool Send(std::list<Client>::iterator client, const std::string& message);

 private:
  int listener_;
  std::list<Client> clients_;

  const int kRecvBuffSize = 1024;
};