#pragma once

#include <memory>
#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>

namespace rsl {

struct Activity {
  std::string state = "A test";
  std::string details = "A placeholder";
  struct Timestamps {
    int64_t start = 0;
    int64_t end = 0;
  };
  Timestamps timestamps;
  struct Assets {
    std::string large_image = "large-image";
    std::string large_text = "Large text";
  };
  Assets assets;
  struct Button {
    std::string text = "A button";
    std::string link = "https://github.com";
  };
  std::vector<Button> buttons{Button{}};
};

class DiscordIpcClient {
public:
  DiscordIpcClient(std::string_view application_id);
  DiscordIpcClient(DiscordIpcClient&&);
  DiscordIpcClient(const DiscordIpcClient&) = delete;
  ~DiscordIpcClient();

  bool connect();
  bool connected() const;
  void disconnect();

  void set_activity(const Activity& activity);
  void test();

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace rsl
