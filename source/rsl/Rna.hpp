#pragma once

#include <core/common.h>
#include <span>
#include <string.h>

namespace rsl {

//! Describes basic types
enum class RNA { Bool, S32, U32, Short, UShort, Byte, SByte, Float };

template <typename T> struct ToType {};

template <> struct ToType<float> { constexpr static RNA v = RNA::Float; };
template <> struct ToType<bool> { constexpr static RNA v = RNA::Bool; };
template <> struct ToType<s32> { constexpr static RNA v = RNA::S32; };
template <> struct ToType<u32> { constexpr static RNA v = RNA::U32; };
template <> struct ToType<s16> { constexpr static RNA v = RNA::Short; };
template <> struct ToType<u16> { constexpr static RNA v = RNA::UShort; };
template <> struct ToType<s8> { constexpr static RNA v = RNA::SByte; };
template <> struct ToType<u8> { constexpr static RNA v = RNA::Byte; };

//! Convert a typename to a type enum
template <typename T> inline constexpr RNA to_rna = ToType<T>::v;

//! std::any, but only for 32-bit RNA primitive types
struct Any32 {
  RNA type;
  f64 value; // 52-bit fraction, able to hold any 32-bit number
};

template <typename T> inline Any32 ReadAny(std::span<const u8> bytes) {
  assert(bytes.size() >= sizeof(T));
  if (bytes.size() < sizeof(T))
    return Any32{.type = to_rna<T>, .value = 0.0};

  T tmp;
  memcpy(&tmp, bytes.data(), sizeof(T));
  return Any32{.type = to_rna<T>, .value = static_cast<f64>(tmp)};
}

template <typename T>
inline void WriteAny(std::span<u8> bytes, const Any32& any) {
  assert(bytes.size() >= sizeof(T));
  if (bytes.size() < sizeof(T))
    return;

  T value = static_cast<T>(any.value);
  memcpy(bytes.data(), &value, sizeof(T));
}
/*
auto any = ReadAny<float>(buf);
any.value ++;
WriteAny<float>(buf, any);
*/
inline Any32 ReadAny(std::span<const u8> bytes, RNA type) {
  switch (type) {
  case RNA::Bool:
    return ReadAny<bool>(bytes);
  case RNA::Byte:
    return ReadAny<u8>(bytes);
  case RNA::SByte:
    return ReadAny<s8>(bytes);
  case RNA::UShort:
    return ReadAny<u16>(bytes);
  case RNA::Short:
    return ReadAny<s16>(bytes);
  case RNA::U32:
    return ReadAny<u32>(bytes);
  case RNA::S32:
    return ReadAny<s32>(bytes);
  case RNA::Float:
    return ReadAny<f32>(bytes);
  }
}
inline void WriteAny(std::span<u8> bytes, const Any32& any) {
  switch (any.type) {
  case RNA::Bool:
    WriteAny<bool>(bytes, any);
    return;
  case RNA::Byte:
    WriteAny<u8>(bytes, any);
    return;
  case RNA::SByte:
    WriteAny<s8>(bytes, any);
    return;
  case RNA::UShort:
    WriteAny<u16>(bytes, any);
    return;
  case RNA::Short:
    WriteAny<s16>(bytes, any);
    return;
  case RNA::U32:
    WriteAny<u32>(bytes, any);
    return;
  case RNA::S32:
    WriteAny<s32>(bytes, any);
    return;
  case RNA::Float:
    WriteAny<f32>(bytes, any);
    return;
  }
}

template <typename T> std::span<const u8> BytesOf(const T& obj) {
  return {reinterpret_cast<const u8*>(&obj), sizeof(obj)};
}
template <typename T> std::span<u8> BytesOf(T& obj) {
  return {reinterpret_cast<u8*>(&obj), sizeof(obj)};
}

} // namespace rsl
