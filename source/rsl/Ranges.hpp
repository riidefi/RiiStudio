#pragma once

#include <ranges>
#include <vector>

namespace rsl {

template <typename T> static bool RangeIsHomogenous(const T& range) {
  return std::adjacent_find(range.begin(), range.end(),
                            std::not_equal_to<>()) == range.end();
}

template <typename T> struct Collector {};
template <size_t N, typename T> struct ArrayCollector {};

// A-la C#
template <typename T = void> static auto ToList() { return Collector<T>{}; }
template <size_t N, typename T = void> static auto ToArray() {
  return ArrayCollector<N, T>{};
}

} // namespace rsl

template <typename T> static auto operator|(auto&& range, rsl::Collector<T>&&) {
  // Value of each item in the range
  using FallbackValueT = typename decltype(range.begin())::value_type;
  // If the typename T overload is picked, use it; otherwise default
  constexpr bool IsCustomValueT = !std::is_void_v<T>;
  using ValueT = std::conditional_t<IsCustomValueT, T, FallbackValueT>;
  return std::vector<ValueT>{range.begin(), range.end()};
}
template <size_t N, typename T>
static auto operator|(auto&& range, rsl::ArrayCollector<N, T>&&) {
  // Value of each item in the range
  using FallbackValueT = typename decltype(range.begin())::value_type;
  // If the typename T overload is picked, use it; otherwise default
  constexpr bool IsCustomValueT = !std::is_void_v<T>;
  using ValueT = std::conditional_t<IsCustomValueT, T, FallbackValueT>;

  auto size = std::ranges::size(range);
  std::array<ValueT, N> arr;
  std::copy_n(range.begin(), std::min(size, arr.size()), arr.begin());
  ValueT dv{};
  std::fill(arr.begin() + std::min(size, arr.size()), arr.end(), dv);
  return arr;
}

namespace rsl {

// Adapted from https://stackoverflow.com/a/28769484
template <typename T, typename TIter = decltype(std::begin(std::declval<T>())),
          typename = decltype(std::end(std::declval<T>()))>
constexpr static auto enumerate(T&& iterable) {
  struct iterator {
    typedef iterator self_type;
    typedef std::tuple<size_t, typename TIter::value_type> value_type;
    typedef value_type& reference;
    typedef value_type* pointer;
    typedef std::forward_iterator_tag iterator_category;
    typedef std::ptrdiff_t difference_type;
    size_t i;
    TIter iter;
    bool operator==(const iterator& other) const { return iter == other.iter; }
    bool operator!=(const iterator& other) const { return iter != other.iter; }
    iterator& operator++() {
      ++i;
      ++iter;
      return *this;
    }
    iterator operator++(int) {
      iterator it = *this;
      ++it;
      return it;
    }
    value_type operator*() const { return std::tie(i, *iter); }
  };
  struct iterable_wrapper {
    T iterable;
    auto begin() { return iterator{0, std::begin(iterable)}; }
    auto end() { return iterator{0, std::end(iterable)}; }
  };
  return iterable_wrapper{std::forward<T>(iterable)};
}

struct Enumerator {};

constexpr static Enumerator enumerate() { return {}; }

} // namespace rsl

static auto operator|(auto&& range, rsl::Enumerator&&) {
  return rsl::enumerate(std::move(range));
}
