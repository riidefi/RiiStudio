#include "DeltaTime.hpp"

namespace riistudio::lvl {

float DeltaTimer::tick() {
  auto now = std::chrono::steady_clock{}.now();
  auto then = last_time;

  if (!has_time) {
    // TODO: Is this the ideal default behavior?
    last_time = now;
    has_time = true;
    return 1.0f / 60.0f;
  }

  last_time = now;
  // TODO: Probably a better way to write this
  using ms = std::chrono::duration<double, std::milli>;
  return std::chrono::duration_cast<ms>(now - then).count() / 1000.0f;
}

} // namespace riistudio::lvl
