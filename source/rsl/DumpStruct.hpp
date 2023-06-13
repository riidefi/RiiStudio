#pragma once

#include <stdio.h>
#include <string>

namespace rsl {

static std::string DumpStruct(auto& s) {
#ifdef __clang__
  std::string result;
  auto printer = [&](auto&&... args) {
    char buf[256];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
    snprintf(buf, sizeof(buf), args...);
#pragma clang diagnostic pop
    result += buf;
  };
  __builtin_dump_struct(&s, printer);
  return result;
#else
  return "UNSUPPORTED COMPILER";
#endif
}

} // namespace rsl
