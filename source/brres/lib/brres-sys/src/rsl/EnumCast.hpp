#pragma once

#include <core/common.h>
#include <rsl/Ranges.hpp>
#include <vendor/magic_enum/magic_enum.hpp>

namespace rsl {

static inline std::string EnumError(u32 bad, auto&& good) {
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
static inline std::string EnumError(std::string_view bad, auto&& good) {
  auto values_printed = rsl::join(good | std::views::transform([](auto x) {
                                    auto [id, name] = x;
                                    return std::string(name);
                                  }),
                                  ", ");
  return std::format(
      "Invalid enum value. Expected one of ({}). Instead saw \"{}\".",
      values_printed, bad, bad);
}

template <typename E>
inline std::expected<E, std::string> enum_cast(u32 candidate) {
  auto as_enum = magic_enum::enum_cast<E>(candidate);
  if (!as_enum.has_value()) {
    auto values = magic_enum::enum_entries<E>();
    auto msg = EnumError(candidate, values);
    return RSL_UNEXPECTED(msg);
  }
  return *as_enum;
}

template <typename E>
inline std::expected<E, std::string> enum_cast(std::string_view candidate) {
  auto as_enum = magic_enum::enum_cast<E>(candidate);
  if (!as_enum.has_value()) {
    auto values = magic_enum::enum_entries<E>();
    auto msg = EnumError(candidate, values);
    return RSL_UNEXPECTED(msg);
  }
  return *as_enum;
}

} // namespace rsl
