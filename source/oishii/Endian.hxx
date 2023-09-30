#pragma once

#include <bit>
#include <stdint.h>

namespace oishii {

template <typename T1, typename T2> union enumCastHelper {
  // Switch above means this always is instantiated
  // static_assert(sizeof(T1) == sizeof(T2), "Sizes of types must match.");
  T1 _t;
  T2 _u;

  enumCastHelper(T1 _) : _t(_) {}
};

#if OISHII_PLATFORM_LE == 1
#define MAKE_BE32(x) std::byteswap(x)
#define MAKE_LE32(x) x
#else
#define MAKE_BE32(x) x
#define MAKE_LE32(x) std::byteswap(x)
#endif

//! @brief Fast endian swapping.
//!
//! @tparam T Type of value to swap. Must be sized 1, 2, or 4
//!
//! @return T endian swapped.
template <typename T> inline T swapEndian(T v) {
  static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4,
                "T must of size 1, 2, or 4");

  switch (sizeof(T)) {
  case 4: {
    enumCastHelper<T, uint32_t> tmp(v);
    tmp._u = std::byteswap(tmp._u);
    return tmp._t;
  }
  case 2: {
    enumCastHelper<T, uint16_t> tmp(v);
    tmp._u = std::byteswap(tmp._u);
    return tmp._t;
  }
  case 1: {
    return v;
  }
  }

  // Never reached
  return T{};
}

enum class EndianSelect {
  Current, // Grab current endian
  // Explicitly use endian
  Big,
  Little
};

template <uint32_t size> struct integral_of_equal_size;

template <> struct integral_of_equal_size<1> {
  using type = uint8_t;
};

template <> struct integral_of_equal_size<2> {
  using type = uint16_t;
};

template <> struct integral_of_equal_size<4> {
  using type = uint32_t;
};

template <typename T>
using integral_of_equal_size_t =
    typename integral_of_equal_size<sizeof(T)>::type;

template <typename T, EndianSelect E = EndianSelect::Current>
inline T endianDecode(T val, std::endian fileEndian) {
  if constexpr (E == EndianSelect::Big) {
    return std::endian::native != std::endian::big ? swapEndian<T>(val) : val;
  } else if constexpr (E == EndianSelect::Little) {
    return std::endian::native != std::endian::little ? swapEndian<T>(val)
                                                      : val;
  } else if constexpr (E == EndianSelect::Current) {
    return std::endian::native != fileEndian ? swapEndian<T>(val) : val;
  }

  return val;
}

constexpr uint32_t roundDown(uint32_t in, uint32_t align) {
  return align ? in & ~(align - 1) : in;
}
constexpr uint32_t roundUp(uint32_t in, uint32_t align) {
  return align ? roundDown(in + (align - 1), align) : in;
}

} // namespace oishii
