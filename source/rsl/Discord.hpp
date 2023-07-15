#pragma once

#include <memory>
#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>

#include "c-discord-rich-presence/include/discord_rpc.h"

namespace rsl {

using Activity = discord_rpc::Activity;

class DiscordIpcClient {
public:
  DiscordIpcClient(std::string_view application_id);
  DiscordIpcClient(DiscordIpcClient&&);
  DiscordIpcClient(const DiscordIpcClient&) = delete;
  ~DiscordIpcClient();

  bool connect();
  bool connected() const;
  void disconnect();

  void set_activity(const discord_rpc::Activity& activity);
  void test();

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace rsl
