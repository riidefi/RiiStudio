#pragma once

#ifndef _WIN32
#include <signal.h>
#endif

namespace rsl {

static inline void debug_break() {
#ifdef _WIN32
  __debugbreak();
#else
  raise(SIGTRAP);
#endif
}

} // namespace rsl
