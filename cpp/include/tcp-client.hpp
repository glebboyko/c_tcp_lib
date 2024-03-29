#pragma once

#define BLOCK_SIZE 1024
#define MS_RECV_TIMEOUT 1000

#include <list>
#include <mutex>
#include <optional>
#include <queue>
#include <semaphore>
#include <sstream>
#include <string>
#include <thread>

#include "tcp-supply.hpp"

namespace TCP {

class TcpServer;

class TcpClient {
 public:
  TcpClient() noexcept = default;
  TcpClient(const char* addr, int port, int ms_ping_threshold,
            int ms_loop_period, logging_foo f_logger = LoggerCap);
  TcpClient(const char* addr, int port, int ms_ping_threshold,
            logging_foo f_logger = LoggerCap);
  TcpClient(const char* addr, int port, logging_foo f_logger = LoggerCap);
  TcpClient(TcpClient&&) noexcept;
  ~TcpClient();

  TcpClient& operator=(TcpClient&&);

  void Connect(const char* addr, int port, int ms_ping_threshold,
               int ms_loop_period, logging_foo f_logger = LoggerCap);
  void Connect(const char* addr, int port, int ms_ping_threshold,
               logging_foo f_logger = LoggerCap);
  void Connect(const char* addr, int port, logging_foo f_logger = LoggerCap);

  template <typename... Args>
  void Send(const Args&... args) {
    LClient logger(LClient::FSend, this, logger_);
    logger.Log("Starting sending method. Checking is peer connected", Debug);
    if (!IsConnected()) {
      logger.Log("Peer is not connected", Warning);
      throw TcpException(TcpException::ConnectionBreak, logger_);
    }
    logger.Log("Peer is connected", Debug);

    logger.Log("Getting string from args", Debug);
    std::string input;
    FromArgs(input, args...);
    logger.Log("Sending message", Debug);
    StrSend(input, logger);
    logger.Log("Message sent", Info);
  }

  std::string RecvStr(int ms_timeout);

  template <typename... Args>
  bool Receive(int ms_timeout, Args&... args) {
    LClient logger(LClient::FRecv, this, logger_);

    auto recv_str = RecvStr(ms_timeout);
    if (recv_str.empty()) {
      return false;
    }

    logger.Log("Setting args from string", Debug);
    std::stringstream stream;
    stream << recv_str;
    ToArgs(stream, args...);
    logger.Log("Message received", Info);
    return true;
  }

  void StopClient() noexcept;
  bool IsAvailable();
  bool IsConnected() noexcept;

  int GetPing();

  int GetMsPingThreshold() const noexcept;

 private:
  int main_socket_;
  int heartbeat_socket_;

  int ping_threshold_;
  int loop_period_;

  std::thread heartbeat_thread_;

  TcpClient** this_pointer_ = nullptr;
  std::mutex* this_mutex_ = nullptr;

  bool is_active_ = false;

  int ms_ping_ = 0;

  logging_foo logger_ = LoggerCap;

  TcpClient(int heartbeat_socket, int main_socket, int ping_threshold,
            int loop_period, logging_foo f_logger);

  static void HeartBeatClient(TcpClient** this_pointer,
                              std::mutex* this_mutex) noexcept;
  static void HeartBeatServer(TcpClient** this_pointer,
                              std::mutex* this_mutex) noexcept;

  // From string to args
  template <IFriendly T>
  void ToArg(std::stringstream& stream, T& var) {
    if (stream.rdbuf()->in_avail() == 0) {
      return;
    }
    stream >> var;
  }

  template <typename T>
    requires(!IFriendly<T>)
  void ToArg(std::stringstream& stream, T& var) {
    if (stream.rdbuf()->in_avail() == 0) {
      return;
    }
    size_t cont_size;
    stream >> cont_size;

    for (size_t i = 0; i < cont_size; ++i) {
      typename T::value_type val;
      ToArg(stream, val);
      var.push_back(std::move(val));
    }
  }

  void ToArgs(std::stringstream& stream);
  template <typename Head, typename... Tail>
  void ToArgs(std::stringstream& stream, Head& head, Tail&... tail) {
    ToArg(stream, head);
    if (stream.eof()) {
      return;
    }
    ToArgs(stream, tail...);
  }

  // From args to string
  template <OFriendly T>
  void FromArg(std::string& output, const T& var) {
    std::stringstream stream;
    stream << var;
    std::string str = stream.str();

    if (!output.empty()) {
      output.push_back(' ');
    }
    output += str;
  }

  template <typename T>
    requires(!OFriendly<T>)
  void FromArg(std::string& output, const T& var) {
    if (!output.empty()) {
      output.push_back(' ');
    }
    output += std::to_string(var.size());

    for (auto iter = var.begin(); iter != var.end(); ++iter) {
      FromArg(output, *iter);
    }
  }

  void FromArgs(std::string& output);
  template <typename Head, typename... Tail>
  void FromArgs(std::string& output, const Head& head, const Tail&... tail) {
    FromArg(output, head);

    FromArgs(output, tail...);
  }

  std::string StrRecv(int ms_timeout, Logger& logger);
  void StrSend(const std::string& message, Logger& logger);

  void CheckReceiveError();

  friend TcpServer;
};

}  // namespace TCP