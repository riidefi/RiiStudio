#pragma once

#include <core/common.h>

#define ASIO_NO_EXCEPTIONS
#include <vendor/asio.hpp>

#include <iostream>

namespace asio::detail {
template <typename Exception> void throw_exception(const Exception& e) {
  std::cerr << "[ASIO] FATAL: " << e.what() << std::endl;
  std::terminate();
}
} // namespace asio::detail

namespace riistudio::net {

using asio::ip::tcp;

class TcpSocket {
public:
  TcpSocket(asio::io_context& io) : mSocket(io), mIo(io) {}
  ~TcpSocket() {
    if (mConnected)
      disconnect();
  }
  TcpSocket(const TcpSocket&) = delete;
  TcpSocket(TcpSocket&&) = delete;

  void connect(unsigned port) {
    assert(!mConnected);

    tcp::acceptor accepter(mIo, tcp::endpoint(tcp::v4(), port));
    accepter.accept(mSocket);
    mConnected = true;
  }

  void disconnect() {
    assert(mConnected);

    asio::error_code ec;
    mSocket.shutdown(tcp::socket::shutdown_both, ec);
    if (ec) {
      DebugReport("[ERROR] Failed to shutdown socket\n");
    }
    mSocket.close();
    mConnected = false;
  }

  void sendBytes(const void* data, size_t len) {
    assert(mConnected);

    u32 len_ = len;
    asio::write(mSocket, asio::buffer(&len_, 4));
    asio::write(mSocket, asio::buffer(data, len));
  }

  void receiveBytes(std::vector<u8>& out) {
    assert(mConnected);

    u32 len_ = 0;
    mSocket.read_some(asio::buffer(&len_, 4));

    if (len_ == 0) {
      out.clear();
      return;
    }

    out.resize(len_);
    mSocket.read_some(asio::buffer(out.data(), out.size()));
  }

  void sendStringLine(std::string str) {
    asio::write(mSocket, asio::buffer(str + "\n"));
  }
  std::string receiveStringLine() {
    asio::streambuf buf;
    asio::read_until(mSocket, buf, "\n");
    return asio::buffer_cast<const char*>(buf.data());
  }

private:
  tcp::socket mSocket;
  asio::io_context& mIo;
  bool mConnected = false;
};

} // namespace riistudio::net
