#include "Discord.hpp"

#include <cassert>
#include <optional>

#include "Log.hpp"

#include <string.h>

#include "c-discord-rich-presence/include/discord_rpc.h"

namespace rsl {

using DISCORD_DELETER = decltype([](::C_RPC* x) {
#ifndef __EMSCRIPTEN__
  rsl_rpc_destroy(x);
#endif
});
using rpc_pointer = std::unique_ptr<::C_RPC, DISCORD_DELETER>;

class DiscordIpcClient::Impl {
private:
  Impl(::C_RPC& ptr) : m_handle(&ptr) {}

public:
  static std::optional<Impl> create(const char* application_id) {
#ifdef __EMSCRIPTEN__
    ::C_RPC* ptr = nullptr;
#else
    ::C_RPC* ptr = rsl_rpc_create(application_id);
#endif
    if (ptr == nullptr) {
      return std::nullopt;
    }
    return Impl(*ptr);
  }
  Impl(const Impl&) = delete;
  Impl(Impl&& rhs) = default;
  ~Impl() = default;

  ::C_RPC& handle() { return *m_handle; }

private:
  rpc_pointer m_handle;

public:
  bool m_connected = false;
};

DiscordIpcClient::DiscordIpcClient(std::string_view application_id) {
  auto s = std::string(application_id);
  auto h = Impl::create(s.c_str());
  if (h.has_value()) {
    m_impl = std::make_unique<Impl>(std::move(*h));
  }
}
DiscordIpcClient::DiscordIpcClient(DiscordIpcClient&& rhs) {
  m_impl = std::move(rhs.m_impl);
}
DiscordIpcClient::~DiscordIpcClient() {
  if (m_impl && m_impl->m_connected) {
    disconnect();
  }
}
bool DiscordIpcClient::connect() {
  if (m_impl == nullptr) {
    rsl::warn("[rsl::DiscordIpcClient] connect() failed. No socket");
    return false;
  }
  if (m_impl->m_connected) {
    // Already connected
    rsl::warn("[rsl::DiscordIpcClient] connect() - Already connected");
    return true;
  }
#ifdef __EMSCRIPTEN__
  return false;
#else
  const int err = rsl_rpc_connect(&m_impl->handle());
  if (err != 0) {
    return false;
  }
  m_impl->m_connected = true;
  return true;
#endif
}
bool DiscordIpcClient::connected() const {
  return m_impl != nullptr && m_impl->m_connected;
}
void DiscordIpcClient::disconnect() {
#ifndef __EMSCRIPTEN__
  if (m_impl != nullptr && m_impl->m_connected) {
    rsl_rpc_disconnect(&m_impl->handle());
    m_impl->m_connected = false;
  }
#endif
}
void DiscordIpcClient::set_activity(const discord_rpc::Activity& activity) {
  if (m_impl == nullptr) {
    rsl::error("[rsl::DiscordIpcClient] set_activity failed. No socket exists");
    return;
  }
  if (!m_impl->m_connected) {
    rsl::error("[rsl::DiscordIpcClient] set_activity failed. Not connected to "
               "Discord");
    return;
  }
#ifndef __EMSCRIPTEN__
  auto* a = discord_rpc::create_activity(activity);
  rsl_rpc_set_activity(&m_impl->handle(), a);
  discord_rpc::destroy_activity(a);
#endif
}
void DiscordIpcClient::test() {
#ifndef __EMSCRIPTEN__
  if (m_impl != nullptr) {
    rsl_rpc_test(&m_impl->handle());
  }
#endif
}

} // namespace rsl
