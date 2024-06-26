#pragma once

#include <map>
#include <mutex>
#include <queue>
#include <semaphore>
#include <thread>

#include "tcp-client.hpp"

namespace TCP {

class TcpServer {
 public:
  TcpServer(int port, int ms_ping_threshold, int ms_loop_period,
            logging_foo f_logger = LoggerCap);
  TcpServer(int port, int ms_ping_threshold, logging_foo f_logger = LoggerCap);
  TcpServer(int port, logging_foo f_logger = LoggerCap);
  ~TcpServer();

  TcpClient AcceptConnection();

  void CloseListener() noexcept;
  bool IsListenerOpen() const noexcept;

 private:
  static const int kMaxClientLength = 1024;

  int listener_;
  bool is_active_ = true;
  int port_;

  int ping_threshold_;
  int loop_period_;

  std::queue<TcpClient> accepted_;
  std::mutex accept_mutex_;
  std::counting_semaphore<kMaxClientLength> accepter_semaphore_ =
      std::counting_semaphore<kMaxClientLength>(0);

  std::map<uint64_t, int> uncomplete_client_;

  logging_foo logger_;

  std::thread accept_thread_;
  void AcceptLoop() noexcept;

  void ConnectListener();
};

}  // namespace TCP