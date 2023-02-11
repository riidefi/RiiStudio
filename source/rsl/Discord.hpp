#pragma once

#include <memory>
#include <string_view>

namespace rsl {

class DiscordIpcClient {
public:
  DiscordIpcClient(std::string_view application_id);
  DiscordIpcClient(DiscordIpcClient&&);
  DiscordIpcClient(const DiscordIpcClient&) = delete;
  ~DiscordIpcClient();

  void connect();
  void test();

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace rsl
