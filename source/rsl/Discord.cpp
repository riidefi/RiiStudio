#include "Discord.hpp"

#include <cassert>
#include <optional>
#include <string>

extern "C" {

struct C_RPC;

C_RPC* rsl_rpc_create(const char* application_id);
void rsl_rpc_destroy(C_RPC*);

void rsl_rpc_connect(C_RPC*);
// Sets dummy status
void rsl_rpc_test(C_RPC*);
}

namespace rsl {

using DISCORD_DELETER = decltype([](::C_RPC* x) { rsl_rpc_destroy(x); });
using rpc_pointer = std::unique_ptr<::C_RPC, DISCORD_DELETER>;

class DiscordIpcClient::Impl {
private:
  Impl(::C_RPC& ptr) : m_handle(&ptr) {}

public:
  static std::optional<Impl> create(const char* application_id) {
    ::C_RPC* ptr = rsl_rpc_create(application_id);
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
DiscordIpcClient::~DiscordIpcClient() = default;
void DiscordIpcClient::connect() {
  if (m_impl != nullptr) {
    rsl_rpc_connect(&m_impl->handle());
  }
}
void DiscordIpcClient::test() {
  if (m_impl != nullptr) {
    rsl_rpc_test(&m_impl->handle());
  }
}

} // namespace rsl
