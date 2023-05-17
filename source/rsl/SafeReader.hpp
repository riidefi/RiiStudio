#pragma once

#include <core/common.h>
#include <oishii/reader/binary_reader.hxx>
#include <rsl/Ranges.hpp>
#ifndef __APPLE__
#include <stacktrace>
#endif
#include <vendor/magic_enum/magic_enum.hpp>

namespace rsl {

inline std::string EnumError(u32 bad, auto&& good) {
  auto values_printed =
      rsl::join(good | std::views::transform([](auto x) {
                  auto [id, name] = x;
                  auto id_raw = static_cast<u32>(id);
                  return std::format("{}={}=0x{:x}", name, id_raw, id_raw);
                }),
                ", ");
  return std::format(
      "Invalid enum value. Expected one of ({}). Instead saw {} (0x{:x}).",
      values_printed, bad, bad);
}

template <typename E>
inline std::expected<E, std::string> enum_cast(u32 candidate) {
  auto as_enum = magic_enum::enum_cast<E>(candidate);
  if (!as_enum.has_value()) {
    auto values = magic_enum::enum_entries<E>();
    auto msg = EnumError(candidate, values);
    return std::unexpected(msg);
  }
  return *as_enum;
}

template <typename T, typename F>
std::expected<T, std::string> checked_cast(const F& f) {
  static_assert(std::is_integral_v<T>);
  static_assert(std::is_integral_v<F>);
  if (static_cast<F>(static_cast<T>(f)) != f) {
    return std::unexpected(std::format(
        "Cannot fit value {} in F (truncates to {})", f, static_cast<T>(f)));
  }
  return static_cast<T>(f);
}

class SafeReader {
public:
  template <typename T> using Result = std::expected<T, std::string>;

  SafeReader(oishii::BinaryReader& reader) : mReader(reader) {}

  // Slicing shouldn't fail, python approach. Just returns a null span.
  auto slice(u32 ofs = 0) -> std::span<const u8>;

  // Doesn't ever fail
  void seekSet(u32 pos);
  auto tell() const -> u32;

  auto F32() -> Result<f32>;
  auto U32() -> Result<u32>;
  auto S32() -> Result<s32>;
  auto U16() -> Result<u16>;
  auto S16() -> Result<s16>;
  auto U8() -> Result<u8>;
  auto S8() -> Result<s8>;

  template <u32 n> auto S32s() -> Result<std::array<s32, n>> {
    std::array<s32, n> result;
    for (auto& r : result) {
      r = TRY(S32());
    }
    return result;
  }

  // TODO: Maybe some subletting system for unaligned contexts?
private:
  static inline constexpr auto Cur = oishii::EndianSelect::Current;

public:
  auto F32NoAlign() -> Result<f32>;
  auto U32NoAlign() -> Result<u32>;
  auto S32NoAlign() -> Result<s32>;
  auto U16NoAlign() -> Result<u16>;
  auto S16NoAlign() -> Result<s16>;
  auto U8NoAlign() -> Result<u8>;
  auto S8NoAlign() -> Result<s8>;

  template <typename T, typename E> auto Enum() -> Result<E> {
    static_assert(std::is_integral_v<T>);
    static_assert(sizeof(T) <= sizeof(u32));

    auto u = TRY(mReader.tryRead<T>());
    auto as_enum = enum_cast<E>(u);
    if (!as_enum.has_value()) {
      mReader.warnAt(as_enum.error().c_str(), mReader.tell() - sizeof(T),
                     mReader.tell());
#if defined(_WIN32)
      auto cur = std::stacktrace::current();
#else
      auto cur = 0;
#endif
      return std::unexpected(std::format("{}\nStacktrace:\n{}", as_enum.error(),
                                         std::to_string(cur)));
    }
    return *as_enum;
  }

  template <typename E> auto Enum32() -> Result<E> { return Enum<u32, E>(); }
  template <typename E> auto Enum16() -> Result<E> { return Enum<u16, E>(); }
  template <typename E> auto Enum8() -> Result<E> { return Enum<u8, E>(); }

  auto Bool8() -> Result<bool>;

  template <size_t size> auto U8Buffer() -> Result<std::array<u8, size>> {
    auto buf = TRY(mReader.tryReadBuffer<u8>(size));
    return buf | rsl::ToArray<size>();
  }
  template <size_t size> auto CharBuffer() -> Result<std::array<char, size>> {
    auto buf = TRY(mReader.tryReadBuffer<char>(size));
    return buf | rsl::ToArray<size>();
  }

  auto Magic(std::string_view ident) -> Result<std::string_view>;

  Result<std::string> StringOfs32(u32 relative);
  Result<std::string> StringOfs(u32 relative) { return StringOfs32(relative); }

  auto scoped(std::string&& name) {
    return mReader.createScoped(std::move(name));
  }

  oishii::BinaryReader& getUnsafe() { return mReader; }

private:
  oishii::BinaryReader& mReader;
};

} // namespace rsl
