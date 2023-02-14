#pragma once

#include <chrono>

namespace riistudio::lvl {

class DeltaTimer {
public:
  float tick();

private:
  std::chrono::steady_clock::time_point last_time;
  bool has_time = false;
};

} // namespace riistudio::lvl
