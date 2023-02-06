#pragma once

#include <array>
#include <functional>
#include <optional>
#include <rsl/Expected.hpp>
#include <stdint.h>
#include <string_view>

namespace librii::sp {

//! Defines an asynchronous debug client for connecting to a game instance.
struct DebugClient {
  struct Settings {
    std::array<uint8_t, 0x50> bdof;
  };

  enum class Error {
    Timeout,
    Other,
  };

  static void Init();
  static void Deinit();

  static void ConnectAsync(std::string_view ip_port, int timeout_ms,
                           std::function<void(std::optional<Error>)> callback);

  static void SendSettingsAsync(const Settings& settings);

  static void GetSettingsAsync(
      int timeout,
      std::function<void(std::expected<Settings, Error>)> callback);
};

} // namespace librii::sp
