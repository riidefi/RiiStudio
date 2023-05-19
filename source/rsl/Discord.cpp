#include "Discord.hpp"

#include <cassert>
#include <optional>

#include "Log.hpp"

#include <string.h>

extern "C" {

struct C_RPC;

C_RPC* rsl_rpc_create(const char* application_id);
void rsl_rpc_destroy(C_RPC*);

// 0 -> OK, negative -> error
int rsl_rpc_connect(C_RPC*);
// Only valid if prior connect call succeeded
void rsl_rpc_disconnect(C_RPC*);
// Sets a status. Type of ffi::Activity from FFIActivity.hh
void rsl_rpc_set_activity(C_RPC*, void* activity);
// Sets dummy status
void rsl_rpc_test(C_RPC*);
}

struct Timestamps_C {
  int64_t start;
  int64_t end;
};

struct Assets_C {
  const char* large_image;
  const char* large_text;
};

struct Button_C {
  const char* text;
  const char* link;
};

struct ButtonVector_C {
  Button_C* buttons;
  size_t length;
};

struct Activity_C {
  const char* state;
  const char* details;
  Timestamps_C timestamps;
  Assets_C assets;
  ButtonVector_C buttons;
};

// Functions to create and destroy the Activity_C struct and to access the
// fields of the Activity struct

Activity_C* create_activity(const rsl::Activity& activity) {
  Activity_C* activity_c = new Activity_C;

  // Allocate and copy strings for state and details
  activity_c->state = strdup(activity.state.c_str());
  activity_c->details = strdup(activity.details.c_str());

  // Copy timestamps and assets
  activity_c->timestamps.start = activity.timestamps.start;
  activity_c->timestamps.end = activity.timestamps.end;
  activity_c->assets.large_image = strdup(activity.assets.large_image.c_str());
  activity_c->assets.large_text = strdup(activity.assets.large_text.c_str());

  // Allocate and copy button vector
  activity_c->buttons.length = activity.buttons.size();
  activity_c->buttons.buttons = new Button_C[activity_c->buttons.length];
  for (size_t i = 0; i < activity_c->buttons.length; i++) {
    activity_c->buttons.buttons[i].text =
        strdup(activity.buttons[i].text.c_str());
    activity_c->buttons.buttons[i].link =
        strdup(activity.buttons[i].link.c_str());
  }

  return activity_c;
}

void destroy_activity(Activity_C* activity_c) {
  // Free strings
  free((void*)activity_c->state);
  free((void*)activity_c->details);
  free((void*)activity_c->assets.large_image);
  free((void*)activity_c->assets.large_text);

  // Free button vector
  for (size_t i = 0; i < activity_c->buttons.length; i++) {
    free((void*)activity_c->buttons.buttons[i].text);
    free((void*)activity_c->buttons.buttons[i].link);
  }
  delete[] activity_c->buttons.buttons;

  delete activity_c;
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
  const int err = rsl_rpc_connect(&m_impl->handle());
  if (err != 0) {
    return false;
  }
  m_impl->m_connected = true;
  return true;
}
bool DiscordIpcClient::connected() const {
  return m_impl != nullptr && m_impl->m_connected;
}
void DiscordIpcClient::disconnect() {
  if (m_impl != nullptr && m_impl->m_connected) {
    rsl_rpc_disconnect(&m_impl->handle());
    m_impl->m_connected = false;
  }
}
void DiscordIpcClient::set_activity(const Activity& activity) {
  if (m_impl == nullptr) {
    rsl::error("[rsl::DiscordIpcClient] set_activity failed. No socket exists");
    return;
  }
  if (!m_impl->m_connected) {
    rsl::error("[rsl::DiscordIpcClient] set_activity failed. Not connected to "
               "Discord");
    return;
  }
  auto* a = create_activity(activity);
  rsl_rpc_set_activity(&m_impl->handle(), a);
  destroy_activity(a);
}
void DiscordIpcClient::test() {
  if (m_impl != nullptr) {
    rsl_rpc_test(&m_impl->handle());
  }
}

} // namespace rsl
