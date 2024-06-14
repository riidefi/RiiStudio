#pragma once

#include <array>
#include <algorithm>
#include <bit>
#include <span>
#include <stdint.h>
#include <type_traits>

namespace rsl {

using byte_view = std::span<const uint8_t>;

template <typename T> static inline T GetFlipped(T value) {
    auto value_representation = std::bit_cast<std::array<uint8_t, sizeof(T)>>(value);
    std::reverse(value_representation.begin(), value_representation.end());
    return std::bit_cast<T>(value_representation);
}

template <typename T> struct endian_swapped_value {
  static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

  constexpr endian_swapped_value() : mData(T{}) {}
  constexpr endian_swapped_value(T v) : mData(GetFlipped(v)) {}

  constexpr operator T() const { return GetFlipped(mData); }
  constexpr T operator*() const { return GetFlipped(mData); }

private:
  T mData;
};

using bu32 = endian_swapped_value<u32>;
using bs32 = endian_swapped_value<s32>;
using bu16 = endian_swapped_value<u16>;
using bs16 = endian_swapped_value<s16>;
using bf32 = endian_swapped_value<f32>;

static_assert(sizeof(endian_swapped_value<u32>) == sizeof(u32));
static_assert(sizeof(endian_swapped_value<s32>) == sizeof(s32));
static_assert(sizeof(endian_swapped_value<u16>) == sizeof(u16));
static_assert(sizeof(endian_swapped_value<s16>) == sizeof(s16));
static_assert(sizeof(endian_swapped_value<f32>) == sizeof(f32));

template <typename T, typename byte_view_t>
static inline T* buffer_cast(byte_view_t data, unsigned offset = 0) {
  if (offset + sizeof(T) > data.size_bytes())
    return nullptr;

  return reinterpret_cast<T*>(data.data() + offset);
}

//! Unsafe API: Verify the operation before calling
template <typename T> static T load(byte_view data, unsigned offset) {
  assert(offset + sizeof(T) <= data.size_bytes());

  return GetFlipped(
      *reinterpret_cast<const T*>(data.data() + offset));
}

//! Unsafe API: Verify the operation before calling
template <typename T>
static void store(T obj, std::span<uint8_t> data, unsigned offset) {
  assert(offset + sizeof(T) <= data.size_bytes());

  *reinterpret_cast<T*>(data.data() + offset) = GetFlipped(obj);
}

//! Unsafe API: Verify the operation before calling
//! (Writeback form)
template <typename T> static T load_update(byte_view data, unsigned& offset) {
  const T result = load<T>(data, offset);
  offset += sizeof(T);
  return result;
}

namespace pp {

inline float lfs(byte_view data, unsigned offset) {
  return load<float>(data, offset);
}
inline uint32_t lwz(byte_view data, unsigned offset) {
  return load<uint32_t>(data, offset);
}
inline uint16_t lhz(byte_view data, unsigned offset) {
  return load<uint16_t>(data, offset);
}
inline int16_t lha(byte_view data, unsigned offset) {
  return load<int16_t>(data, offset);
}
inline uint8_t lbz(byte_view data, unsigned offset) {
  return load<uint8_t>(data, offset);
}

inline float lfsu(byte_view data, unsigned& offset) {
  return load_update<float>(data, offset);
}
inline uint32_t lwzu(byte_view data, unsigned& offset) {
  return load_update<uint32_t>(data, offset);
}
inline uint16_t lhzu(byte_view data, unsigned& offset) {
  return load_update<uint16_t>(data, offset);
}
inline int16_t lhau(byte_view data, unsigned& offset) {
  return load_update<int16_t>(data, offset);
}
inline uint32_t lbzu(byte_view data, unsigned& offset) {
  return load_update<uint8_t>(data, offset);
}

} // namespace pp

} // namespace rsl
