#pragma once

#include <vector>

namespace rsl {

// Only requires T == T
//
// O(n)
template <typename K, typename V> struct SimpleMap {
  static constexpr size_t npos = -1;

  std::vector<K> keys;
  std::vector<V> values;

  size_t find(const K& x) const {
    auto it = std::find(keys.begin(), keys.end(), x);
    if (it != keys.end())
      return it - keys.begin();

    return npos;
  }

  std::optional<V> get(const K& key) const {
    size_t pos = find(key);
    if (pos != npos) {
      return values[pos];
    }
    return std::nullopt;
  }

  void emplace(const K& key, const V& val) {
    size_t pos = find(key);
    if (pos != npos) {
      values[pos] = val;
      return;
    }

    keys.push_back(key);
    values.push_back(val);
  }

  bool contains(const K& key) const { return find(key) != npos; }

  V operator[](const K& key) const {
    auto pos = find(key);
    assert(pos != npos);
    return values[pos];
  }
};

}