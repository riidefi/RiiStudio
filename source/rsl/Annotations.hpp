#pragma once

#include <algorithm>

// Implementation of "Annotations", zero-length fields with a string in their
// name
namespace annotations_detail {
#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)

template <size_t N> struct StringLiteral {
  constexpr StringLiteral(const char (&str)[N]) { std::copy_n(str, N, value); }

  char value[N];
};

template <const StringLiteral s> struct Annotation {};

#if 0
#if (defined(__clang__) || defined(__linux__)) && !defined(_MSC_VER)
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#endif
#endif

#define NO_UNIQUE_ADDRESS

} // namespace annotations_detail
#define ANNOTATE(s)                                                            \
  NO_UNIQUE_ADDRESS s MACRO_CONCAT(annotation, __COUNTER__) {}
#define ANNOTATE_STR(s) ANNOTATE(annotations_detail::Annotation<s>)
#define ANNOTATE_STR2(s, s2)                                                   \
  ANNOTATE_STR(s);                                                             \
  ANNOTATE_STR(s2)
