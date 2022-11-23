#pragma once

#include <utility>

namespace rsl {

template <typename TFunctor> struct Defer {
  using functor_t = TFunctor;

  TFunctor mF;
  constexpr Defer(functor_t&& F) : mF(std::move(F)) {}
  constexpr ~Defer() { mF(); }
};

} // namespace rsl

#define RSL_CONCAT_IMPL(x, y) x##y
#define RSL_MACRO_CONCAT(x, y) RSL_CONCAT_IMPL(x, y)
#define RSL_DEFER(f)                                                           \
  rsl::Defer RSL_MACRO_CONCAT(__tmp, __COUNTER__) {                            \
    [&]() { f; }                                                               \
  }
