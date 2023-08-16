#pragma once

#include <future>
#include <rsl/Discord.hpp>
#include <vector>

namespace riistudio::frontend {

//! Implements a high-level API for connecting to Discord and setting a status
//! update. Move-only type.
class DiscordRPCManager {
public:
  DiscordRPCManager() = default;
  DiscordRPCManager(DiscordRPCManager&&) = default;
  DiscordRPCManager(const DiscordRPCManager&) = delete;
  ~DiscordRPCManager() = default;

  //! Non-blocking. Performs the connection and sets the initial status.
  void connect() {
#ifndef __EMSCRIPTEN__
    mTaskQueue.push_back(std::async(std::launch::async, [&] {
      mDiscordRpc.connect();
      mDiscordRpc.set_activity(mActivity);
    }));
#endif
  }

  //! If the status has changed, send a blocking request.
  void setStatus(std::string&& status) {
#ifndef __EMSCRIPTEN__
    if (mActivity.details != status) {
      mActivity.details = std::move(status);
      mDiscordRpc.set_activity(mActivity);
    }
#endif
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
  std::vector<std::future<void>> mTaskQueue;
};

} // namespace riistudio::frontend
