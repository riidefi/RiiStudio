#pragma once

#include <rsl/Discord.hpp>

namespace riistudio::frontend {

//! Implements a high-level API for connecting to Discord and setting a status
//! update. Move-only type.
class DiscordRPCManager {
public:
  DiscordRPCManager() = default;
  DiscordRPCManager(DiscordRPCManager&&) = default;
  DiscordRPCManager(const DiscordRPCManager&) = delete;
  ~DiscordRPCManager() = default;

  //! Blocking. Performs the connection and sets the initial status.
  void connect() {
    mDiscordRpc.connect();
    mDiscordRpc.set_activity(mActivity);
  }

  //! If the status has changed, send a blocking request.
  void setStatus(std::string&& status) {
    if (mActivity.details != status) {
      mActivity.details = std::move(status);
      mDiscordRpc.set_activity(mActivity);
    }
  }

private:
  rsl::DiscordIpcClient mDiscordRpc{"1074030676648669235"};
  rsl::Activity mActivity{
      .state = "RiiStudio",
      .details = "Idling", // Filled in by editor
      .timestamps =
          {
              .start = time(0),
          },
      .assets = {.large_image = "large-image", .large_text = "Large text"},
      .buttons =
          {
              {
                  .text = "GitHub",
                  .link = "https://github.com/riidefi/RiiStudio",
              },
          },
  };
};

} // namespace riistudio::frontend
