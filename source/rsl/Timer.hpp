#pragma once

#include <chrono>

namespace rsl {

class Timer {
  using clock_t = std::chrono::steady_clock;

public:
  Timer() { reset(); }
  void reset() { start = clock_t::now(); }
  u32 elapsed() {
    auto end = clock_t::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
        .count();
  }

private:
  clock_t::time_point start;
};

} // namespace rsl
