#pragma once

namespace rsl {

static inline void debug_break() {
#ifdef _WIN32
  __debugbreak();
#endif
}

} // namespace rsl
