#pragma once

// As AppleClang doesn't support C++23 std::expected yet.

#include <variant>

namespace rsl {

template <class T, class E> class expected {
public:
  using value_type = T;
  using error_type = E;
  template <typename U> using rebind = expected<U, error_type>;

  expected() = default;
  expected(const expected&) = default;
  expected(expected&&) = default;
  expected(value_type&& t) : mVariant(t) {}
  expected(error_type&& t) : mVariant(t) {}
  // TODO: Implement unexpected
  expected(const error_type& t) : mVariant(t) {}
  ~expected() = default;

  const T* operator->() const { return std::get_if<T>(&mVariant); }
  T* operator->() { return std::get_if<T>(&mVariant); }

  const T& operator*() const {
    const auto* value = std::get_if<T>(&mVariant);
    assert(value != nullptr);
    return *value;
  }
  T& operator*() {
    auto* value = std::get_if<T>(&mVariant);
    assert(value != nullptr);
    return *value;
  }

  explicit operator bool() const { return has_value(); }
  bool has_value() const { return std::get_if<E>(&mVariant) == nullptr; }

  const E& error() const {
    const auto* error = std::get_if<E>(&mVariant);
    assert(error != nullptr);
    return *error;
  }
  E& error() {
    auto* error = std::get_if<E>(&mVariant);
    assert(error != nullptr);
    return *error;
  }

private:
  std::variant<T, E> mVariant;
};

} // namespace rsl
