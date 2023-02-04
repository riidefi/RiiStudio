#pragma once

// As AppleClang doesn't support C++23 std::expected yet.
#if __cpp_lib_expected >= 202202L
#include <expected>
#else
#include <variant>

namespace rsl {

template <class E_> struct unexpected {
  E_ data;

  unexpected() = default;
  unexpected(const E_& e) : data(e) {}
};

template <class T, class E_> class expected {
public:
  using value_type = std::conditional_t<std::is_void_v<T>, int, T>;
  using error_type = unexpected<E_>;
  template <typename U> using rebind = expected<U, error_type>;

  expected() = default;
  expected(const expected&) = default;
  expected(expected&&) = default;
  expected(value_type&& t) : mVariant(std::move(t)) {}
  expected(const value_type& t) : mVariant(t) {}
  expected(error_type&& t) : mVariant(std::move(t)) {}
  expected(const error_type& t) : mVariant(t) {}
  expected(const char* s) : mVariant(std::string(s)) {}
  ~expected() = default;

  expected& operator=(const expected<T, E_>& rhs) {
    mVariant = rhs.mVariant;
    return *this;
  }

  const T* operator->() const { return std::get_if<T>(&mVariant); }
  T* operator->() { return std::get_if<T>(&mVariant); }

  const value_type& operator*() const {
    const auto* value = std::get_if<value_type>(&mVariant);
    assert(value != nullptr);
    return *value;
  }
  value_type& operator*() {
    auto* value = std::get_if<value_type>(&mVariant);
    assert(value != nullptr);
    return *value;
  }

  explicit operator bool() const { return has_value(); }
  bool has_value() const {
    return std::get_if<error_type>(&mVariant) == nullptr;
  }

  const E_& error() const {
    const auto* error = std::get_if<error_type>(&mVariant);
    assert(error != nullptr);
    return error->data;
  }
  E_& error() {
    auto* error = std::get_if<error_type>(&mVariant);
    assert(error != nullptr);
    return error->data;
  }

  const value_type& value_or(const value_type& other) const {
    return has_value() ? *std::get_if<value_type>(&mVariant) : other;
  }

private:
  std::variant<value_type, error_type> mVariant;
};

} // namespace rsl

namespace std {
template <typename T, typename E> using expected = rsl::expected<T, E>;
template <typename T> rsl::unexpected<std::string> unexpected(T&& x) {
  // Using std::string directly over std::remove_cvref_t<decltype(x)> +
  // conversion magic is a hack.
  return rsl::unexpected<std::string>(x);
}
} // namespace std

#define __cpp_lib_expected 202202L

#endif
