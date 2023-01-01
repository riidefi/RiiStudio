#pragma once

#include <algorithm>
#include <core/common.h>
#include <core/util/oishii.hpp>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <rsl/SimpleReader.hpp>
#include <rsl/SmallVector.hpp>
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

template <typename T> struct MergedDataAllocator {
  size_t add(T x) {
    auto it = std::find(data.begin(), data.end(), x);
    if (it != data.end())
      return it - data.begin();

    data.push_back(x);
    return data.size() - 1;
  }

  size_t find(T x) const {
    auto it = std::find(data.begin(), data.end(), x);
    if (it != data.end())
      return it - data.begin();

    assert(!"Cannot find T");
    return 0;
  }

  std::vector<T> data;
};

class RelocWriter {
public:
  struct Reloc {
    std::string from, to;
    std::size_t ofs, sz;
  };

  RelocWriter(oishii::Writer& writer) : mWriter(writer) {}
  RelocWriter(oishii::Writer& writer, const std::string& prefix)
      : mPrefix(prefix), mWriter(writer) {}

  RelocWriter sublet(const std::string& path) {
    return RelocWriter(mWriter, mPrefix + "/" + path);
  }

  // Define a label, associated with the current stream position
  void label(const std::string& id, const std::size_t addr) {
    mLabels.try_emplace(id, addr);
  }
  void label(const std::string& id) { label(id, mWriter.tell()); }
  // Write a relocation, to be filled in by a resolve() call
  template <typename T>
  void writeReloc(const std::string& from, const std::string& to) {
    mRelocs.emplace_back(
        Reloc{.from = from, .to = to, .ofs = mWriter.tell(), .sz = sizeof(T)});
    mWriter.write<T>(static_cast<T>(0), /* checkmatch */ false);
  }
  // Write a relocation, with an associated child frame to be written by
  // writeChildren()
  template <typename T>
  void writeReloc(const std::string& from, const std::string& to,
                  std::function<void(oishii::Writer&)> write) {
    mRelocs.emplace_back(
        Reloc{.from = from, .to = to, .ofs = mWriter.tell(), .sz = sizeof(T)});
    mWriter.write<T>(static_cast<T>(-1));
    mChildren.emplace_back(to, write);
  }
  // Write all child bodies
  void writeChildren() {
    for (auto& child : mChildren) {
      label(child.first);
      child.second(mWriter);
    }
    mChildren.clear();
  }
  // Resolve a single relocation
  void resolve(Reloc& reloc) {
    const auto from = mLabels.find(reloc.from);
    const auto to = mLabels.find(reloc.to);

    int delta = 0;
    if (from == mLabels.end() || to == mLabels.end()) {
      printf("Bad lookup: %s to %s\n", reloc.from.c_str(), reloc.to.c_str());
      return; // come back..
    } else {
      delta = to->second - from->second;
    }

    const auto back = mWriter.tell();

    mWriter.seekSet(reloc.ofs);

    if (reloc.sz == 4) {
      mWriter.write<s32>(delta);
    } else if (reloc.sz == 2) {
      mWriter.write<s16>(delta);
    } else {
      assert(false);
      printf("Invalid reloc size..\n");
    }

    mWriter.seekSet(back);
  }
  // Resolve all relocations
  void resolve() {
    for (auto& reloc : mRelocs)
      resolve(reloc);
    auto it = std::remove_if(mRelocs.begin(), mRelocs.end(), [&](auto& reloc) {
      const auto from = mLabels.find(reloc.from);
      const auto to = mLabels.find(reloc.to);

      return from != mLabels.end() && to != mLabels.end();
    });
    if (it != mRelocs.end()) {
      mRelocs.erase(it);
    }
  }

  auto& getLabels() const { return mLabels; }
  void printLabels() const {
    for (auto& [label, at] : mLabels) {
      const auto uat = static_cast<unsigned>(at);
      printf("%s: 0x%x (%u)\n", label.c_str(), uat, uat);
    }
  }

private:
  std::string mPrefix;

  oishii::Writer& mWriter;
  std::map<std::string, std::size_t> mLabels;

  rsl::small_vector<Reloc, 64> mRelocs;
  rsl::small_vector<
      std::pair<std::string, std::function<void(oishii::Writer&)>>, 16>
      mChildren;
};

} // namespace librii::g3d
