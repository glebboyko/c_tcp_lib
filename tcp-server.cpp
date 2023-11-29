#include "tcp-server.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <list>
#include <string>

TcpServer::TcpServer(int protocol, int port) {
  listener_ = socket(AF_INET, SOCK_STREAM, protocol);
  if (listener_ < 0) {
    throw std::ios_base::failure("cannot create socket");
  }

  sockaddr_in addr = {.sin_family = AF_INET,
                      .sin_port = htons(port),
                      .sin_addr = {htonl(INADDR_ANY)}};

  if (bind(listener_, (sockaddr*)&addr, sizeof(addr)) < 0) {
    close(listener_);
    throw std::ios_base::failure("cannot bind");
  }

  if (listen(listener_, 1) < 0) {
    close(listener_);
    throw std::ios_base::failure("cannot listen");
  }
}

TcpServer::~TcpServer() {
  while (!clients_.empty()) {
    close(clients_.cbegin()->dp_);
    clients_.erase(clients_.cbegin());
  }
  close(listener_);
}

std::list<TcpServer::Client>::iterator TcpServer::AcceptConnection() {
  int client = accept(listener_, NULL, NULL);

  if (client < 0) {
    throw std::ios_base::failure("cannot accept");
  }

  clients_.push_front(Client(client));
  return clients_.begin();
}

void TcpServer::CloseConnection(std::list<Client>::iterator client) {
  close(client->dp_);
  clients_.erase(client);
}

std::string TcpServer::Receive(std::list<Client>::iterator client) {
  std::string received;

  char num_buff[sizeof(int) + 1];
  // receive number of bytes to receive
  int b_recv = recv(client->dp_, &num_buff, sizeof(int) + 1, 0);
  if (b_recv <= 0) {
    throw std::ios_base::failure("cannot receive");
  }
  int b_num;
  sscanf(num_buff, "%d", &b_num);

  // receive message
  char* buff = new char[b_num];
  b_recv = recv(client->dp_, buff, b_num, 0);
  if (b_recv <= 0) {
    delete[] buff;
    throw std::ios_base::failure("cannot receive");
  }
  for (int i = 0; i < b_recv; ++i) {
    received.push_back(buff[i]);
  }
  return received;
}

bool TcpServer::Send(std::list<Client>::iterator client,
                     const std::string& message) {
  // send number of bytes to send
  char num_of_bytes[sizeof(int) + 1];
  sprintf(num_of_bytes, "%d", message.size());
  int b_sent = send(client->dp_, num_of_bytes, sizeof(int) + 1, 0);
  if (b_sent < 0) {
    throw std::ios_base::failure("cannot send");
  }

  // send message
  b_sent = send(client->dp_, message.c_str(), message.size(), 0);
  if (b_sent < 0) {
    throw std::ios_base::failure("cannot send");
  }

  return b_sent == message.size();
}