#pragma once

#include <core/common.h>
#include <optional>
#include <rsl/SimpleReader.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace librii::g3d {

struct CommonHeader {
  u32 fourcc;
  u32 size;
  u32 revision;
  s32 ofs_parent_dict;
};

constexpr u32 CommonHeaderBinSize = 16;

inline std::optional<CommonHeader> ReadCommonHeader(std::span<const u8> data) {
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
inline std::string_view ReadStringPointer(std::span<const u8> bytes,
                                          size_t pointer_location,
                                          size_t addend) {
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

// { 0, 4 } -> Struct+04 is a string pointer relative to struct start
// { 4, 8 } -> Struct+08 is a string pointer relative to Struct+04
struct NameReloc {
  s32 offset_of_delta_reference;   // Relative to start of structure
  s32 offset_of_pointer_in_struct; // Relative to start of structure
  std::string name;

  bool non_volatile = false; // system name, external reference
};

// { 4, 8} -> Rebase(14) -> { 18, 24 }
inline NameReloc RebasedNameReloc(NameReloc child, s32 offset_parent_to_child) {
  child.offset_of_delta_reference += offset_parent_to_child;
  child.offset_of_pointer_in_struct += offset_parent_to_child;

  return child;
}

constexpr u32 NullBlockID = 0xFFFF'FFFF;

struct BlockID {
  u32 data = NullBlockID;
};

struct BlockDistanceConstraint {
  s64 min_distance = std::numeric_limits<s32>::min();
  s64 max_distance = std::numeric_limits<s32>::max();
};

template <typename T>
inline BlockDistanceConstraint CreateRestraintForOffsetType() {
  return BlockDistanceConstraint{.min_distance = std::numeric_limits<T>::min(),
                                 .max_distance = std::numeric_limits<T>::max()};
}

// Returns x such that:
//   x == 0: reachable
//   x < 0: -x below min
//   x > 0: x above max
inline s64 CompareBlockReachable(s64 delta, BlockDistanceConstraint reach) {
  if (delta > reach.max_distance)
    return reach.max_distance - delta;

  if (delta < reach.min_distance)
    return delta - reach.min_distance;

  return 0;
}

struct BlockData {
  u32 size;
  u32 start_align;
};

struct Block {
  BlockID ID;
  BlockData data;

  std::vector<BlockDistanceConstraint> constraints;
};

} // namespace librii::g3d
