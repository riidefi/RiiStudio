#pragma once

#include <string_view>

namespace riistudio {
const char* translateString(std::string_view str);
} // namespace riistudio

inline const char* operator"" _j(const char* str, size_t len) {
  return riistudio::translateString({str, len});
}
