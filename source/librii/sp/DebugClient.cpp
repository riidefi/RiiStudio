#include "DebugClient.hpp"
#include "detail/FFI.h"
#include <memory>

namespace librii::sp {

static std::optional<DebugClient::Error> ConvertError(int result) {
  switch (result) {
  case DEBUGCLIENT_RESULT_OK:
    return std::nullopt;
  case DEBUGCLIENT_RESULT_TIMEOUT:
    return DebugClient::Error::Timeout;
  case DEBUGCLIENT_RESULT_OTHER:
  default:
    return DebugClient::Error::Other;
  }
}

void DebugClient::Init() { debugclient_init(); }
void DebugClient::Deinit() { debugclient_deinit(); }

void DebugClient::ConnectAsync(
    std::string_view ip_port, int timeout_ms,
    std::function<void(std::optional<Error>)> callback) {
  auto my_callback = [](void* user, int result) {
    std::unique_ptr<decltype(callback)> cb(
        reinterpret_cast<decltype(&callback)>(user));
    auto status = ConvertError(result);
    (*cb)(status);
  };
  std::string ip_port_(ip_port);
  debugclient_connect(ip_port_.c_str(), timeout_ms, +my_callback,
                      new std::function(callback));
}

void DebugClient::SendSettingsAsync(const Settings& settings) {
  debugclient_send_settings(reinterpret_cast<const ::C_Settings*>(&settings));
}

void DebugClient::GetSettingsAsync(
    int timeout_ms,
    std::function<void(std::expected<Settings, Error>)> callback) {
  auto my_callback = [](void* user, int result, const ::C_Settings* settings) {
    std::unique_ptr<decltype(callback)> cb(
        reinterpret_cast<decltype(&callback)>(user));
    auto status = ConvertError(result);
    if (status.has_value()) {
#ifdef __APPLE__
      // (*cb)(unexpected2(*status));
#else
      (*cb)(std::unexpected(*status));
#endif
    } else {
      (*cb)(*reinterpret_cast<const Settings*>(settings));
    }
  };
  debugclient_get_settings(timeout_ms, +my_callback,
                           new std::function(callback));
}

} // namespace librii::sp

// PROTOTYPE IMPLEMENTATION
#include <core/util/net.hpp>

class SynchronousDebugClientPrototype {
public:
  static SynchronousDebugClientPrototype& instance();

  void connect(std::string ip_port);
  void sendSettings(const C_Settings& settings);
  C_Settings getSettings();

private:
  static SynchronousDebugClientPrototype sInstance;

  asio::io_context m_io;
  riistudio::net::TcpSocket m_server{m_io};
};

SynchronousDebugClientPrototype SynchronousDebugClientPrototype::sInstance;

enum {
  MSG_SENDSETTINGS = 1,
  MSG_GETSETTINGS = 2,
};

SynchronousDebugClientPrototype& SynchronousDebugClientPrototype::instance() {
  return sInstance;
}

void SynchronousDebugClientPrototype::connect(std::string ip_port) {
  // Ignores ip_port
  m_server.connect(1234);
}

void SynchronousDebugClientPrototype::sendSettings(const C_Settings& settings) {
  s32 msg = MSG_SENDSETTINGS;
  m_server.sendBytes(&msg, sizeof(msg));
  m_server.sendBytes(&settings, sizeof(settings));
}

C_Settings SynchronousDebugClientPrototype::getSettings() {
  s32 msg = MSG_GETSETTINGS;
  m_server.sendBytes(&msg, sizeof(msg));
  std::vector<u8> reply;
  m_server.receiveBytes(reply);
  assert(reply.size() == sizeof(C_Settings));
  C_Settings result{};
  memcpy(&result, reply.data(), sizeof(result));
  return result;
}

std::vector<std::future<void>> sTaskQueue;
std::mutex sTaskMutex;

void debugclient_init(void) {}
void debugclient_deinit(void) { sTaskQueue.clear(); }

void debugclient_connect(const char* ip_port, int timeout,
                         void (*callback)(void* user, int result), void* user) {
  auto task = [=]() {
    SynchronousDebugClientPrototype::instance().connect(ip_port);
    callback(user, DEBUGCLIENT_RESULT_OK);
  };
  auto future = std::async(std::launch::async, task);
  std::unique_lock g(sTaskMutex);
  sTaskQueue.push_back(std::move(future));

  static bool regist = false;
  if (!regist) {
    regist = true;
    atexit(debugclient_deinit);
  }
}
void debugclient_send_settings(const C_Settings* settings) {
  SynchronousDebugClientPrototype::instance().sendSettings(*settings);
}
void debugclient_get_settings(int timeout,
                              void (*callback)(void* user, int result,
                                               const C_Settings* settings),
                              void* user) {
  C_Settings settings =
      SynchronousDebugClientPrototype::instance().getSettings();
  callback(user, DEBUGCLIENT_RESULT_OK, &settings);
}
