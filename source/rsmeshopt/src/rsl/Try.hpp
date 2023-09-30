#pragma once

#include <stdio.h>
#include <string>
#include <type_traits>
#include <utility>

// No <expected> yet
#include <rsl/Expected.hpp>

// clang-format off
//
// ```cpp
//    std::expected<int, Err> GetInt();
//    std::expected<int, Err> Foo() {
//       return TRY(GetInt()) + 5;
//    }
// ```
//
// https://godbolt.org/z/vhxdsbdqG
//
// In particular:
// - Move-only types work
// - Copy-only types work
// - Result<void> types work
//
// Trick to avoid copies via an rvalue-valued rvalue-member function inspired from from SerenityOS https://github.com/SerenityOS/serenity/blob/master/AK/Try.h
// (Thanks to @InusualZ for pointing this out)
//
// (The `MyMove` function is some glue I came up with for the `void` case; perhaps there is a more elegant way?)
//
#if (defined(__clang__) || defined(__GNUC__) || defined(__APPLE__)) && defined(__cpp_lib_remove_cvref) && __cpp_lib_remove_cvref >= 201711L
#define HAS_RUST_TRY
template <typename T> auto MyMove(T&& t) {
  if constexpr (!std::is_void_v<typename std::remove_cvref_t<T>::value_type>) {
    return std::move(*t);
  }
}
#define TRY(...)                                                               \
  ({                                                                           \
    auto&& y = (__VA_ARGS__);                                                  \
    static_assert(!std::is_lvalue_reference_v<decltype(MyMove(y))>);           \
    if (!y) [[unlikely]] {                                                     \
      return RSL_UNEXPECTED(y.error());                                        \
    }                                                                          \
    MyMove(y);                                                                 \
  })
#define BEGINTRY
#define ENDTRY
#else
// #define TRY(...) static_assert(false, "Compiler does not support TRY macro")
#define TRY(...)                                                               \
  [](auto&& x) {                                                               \
    if (!x.has_value()) {                                                      \
      fprintf(stderr, "Fatal error: %s", x.error().c_str());                   \
      throw x.error();                                                         \
    }                                                                          \
    return *x;                                                                 \
  }(__VA_ARGS__)
#define BEGINTRY try {
#define ENDTRY                                                                 \
  }                                                                            \
  catch (std::string s) {                                                      \
    return RSL_UNEXPECTED(s);                                                  \
  }
#endif
// clang-format on
