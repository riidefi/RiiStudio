#pragma once

#include <algorithm>

template <typename A, typename B>
static inline int indexOf(A&& x, B&& y) {
  int index = std::find_if(x.begin(), x.end(),
                           [y](auto& f) { return f.getName() == y; }) -
              x.begin();
  return index >= x.size() ? -1 : index;
}

static inline auto findByName = [](auto&& x, auto&& y) {
  int index = std::find_if(x.begin(), x.end(),
                           [y](auto& f) { return f.getName() == y; }) -
              x.begin();
  return index >= x.size() ? nullptr : &x[index];
};

static inline auto findByName2 = [](auto&& x, auto&& y) {
  int index =
      std::find_if(x.begin(), x.end(), [y](auto& f) { return f.name == y; }) -
      x.begin();
  return index >= x.size() ? nullptr : &x[index];
};
