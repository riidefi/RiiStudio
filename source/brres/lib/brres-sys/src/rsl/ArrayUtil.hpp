#pragma once

#include <algorithm>

#if (defined(__APPLE__) && __cplusplus > 201703L) ||                           \
    (!defined(__APPLE__) && __cpp_lib_expected >= 202202L)
namespace {
template <typename A, typename B> int indexOf(A&& x, B&& y) {
  int index = std::find_if(x.begin(), x.end(),
                           [y](auto& f) { return f.getName() == y; }) -
              x.begin();
  return index >= x.size() ? -1 : index;
};
auto findByName = [](auto&& x, auto&& y) {
  int index = std::find_if(x.begin(), x.end(),
                           [y](auto& f) { return f.getName() == y; }) -
              x.begin();
  return index >= x.size() ? nullptr : &x[index];
};
} // namespace
#endif

auto findByName2 = [](auto&& x, auto&& y) {
  int index =
      std::find_if(x.begin(), x.end(), [y](auto& f) { return f.name == y; }) -
      x.begin();
  return index >= x.size() ? nullptr : &x[index];
};
