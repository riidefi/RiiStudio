#pragma once

#include <core/common.h>
#include <optional>
#include <rsl/SimpleReader.hpp>
#include <span>
#include <string_view>

namespace librii::g3d {

struct CommonHeader {
  u32 fourcc;
  u32 size;
  u32 revision;
  s32 ofs_parent_dict;
};

constexpr u32 CommonHeaderBinSize = 16;

static std::optional<CommonHeader> ReadCommonHeader(std::span<const u8> data) {
  if (data.size_bytes() < 16) {
    return std::nullopt;
  }

  return CommonHeader{.fourcc = rsl::load<u32>(data, 0),
                      .size = rsl::load<u32>(data, 4),
                      .revision = rsl::load<u32>(data, 8),
                      .ofs_parent_dict = rsl::load<s32>(data, 12)};
}

// The in-file pointer will be validated. `pointer_location` must be validated
// by the user
std::string_view ReadStringPointer(std::span<const u8> bytes,
                                   size_t pointer_location, size_t addend) {
  assert(pointer_location < bytes.size_bytes());
  const s32 rel_pointer_value = rsl::load<s32>(bytes, pointer_location);

  if (rel_pointer_value == 0) {
    return "";
  }

  const size_t pointer_value = addend + rel_pointer_value;

  if (pointer_value < bytes.size_bytes() && pointer_value >= 0) {
    return reinterpret_cast<const char*>(bytes.data() + pointer_value);
  }

  return "";
}

} // namespace librii::g3d
