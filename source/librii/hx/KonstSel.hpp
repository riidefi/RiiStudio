#pragma once

#include <algorithm> // std::clamp
#include <librii/gx.h>

namespace librii::hx {

struct KonstSel {
  bool k_constant = true;
  // if k_constant:
  int k_numerator = 8; // [1, 8]; denominator is 8

  // else:
  int k_reg = 0; // k0, k1, k2, k3
  int k_sub = 0; // rgb, rrr, ggg, bbb, aaa. rgb is not valid for alpha stages.
};

template <typename T> KonstSel elevateKonstSel(T x) {
  KonstSel result;
  int ksel = static_cast<int>(x);
  result.k_constant = ksel == std::clamp(ksel, static_cast<int>(T::const_8_8),
                                         static_cast<int>(T::const_1_8));
  result.k_numerator =
      result.k_constant ? 8 - (ksel - static_cast<int>(T::const_8_8)) : -1;
  result.k_reg = !result.k_constant ? (ksel - static_cast<int>(T::k0)) % 5 : -1;
  result.k_sub = !result.k_constant ? (ksel - static_cast<int>(T::k0)) / 5
                                    : -1; // rgba, r, g, b, a
  return result;
}
template <typename T> T lowerKonstSel(KonstSel ksel) {
  if (ksel.k_constant) {
    ksel.k_numerator -= 1;                    // [0, 7]
    ksel.k_numerator = -ksel.k_numerator + 7; // -> [7, 0]
    return static_cast<T>(static_cast<int>(T::const_8_8) + ksel.k_numerator);
  } else {
    return static_cast<T>(static_cast<int>(T::k0) + (ksel.k_sub * 5) +
                          ksel.k_reg);
  }
}

} // namespace librii::hx
