#pragma once

#include <algorithm>        // std::fill
#include <core/common.h>    // assert
#include <string_view>      // std::string_view

namespace rsl {

inline void to_lower(std::string& str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return ::tolower(c); });
}

inline std::string to_lower(std::string_view str) {
  std::string ret = std::string(str);
  to_lower(ret);
  return ret;
}

inline void to_upper(std::string& str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return ::toupper(c); });
}

inline std::string to_upper(std::string_view str) {
  std::string ret = std::string(str);
  to_upper(ret);
  return ret;
}

} // namespace rsl
