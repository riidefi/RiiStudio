#pragma once

#ifdef BUILD_ASAN
#include <sanitizer/lsan_interface.h>
#endif

namespace riistudio {

inline void DoLeakCheck() {
#ifdef BUILD_ASAN
  __lsan_do_leak_check();
#endif
}

} // namespace riistudio
