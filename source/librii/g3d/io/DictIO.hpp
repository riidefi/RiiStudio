#pragma once

#include <core/common.h>
#include <rsl/SimpleReader.hpp>
#include <span>
#include <string_view>
#if __cpp_lib_concepts >= 201907L
#include <concepts>
#endif

namespace librii::g3d {

struct DictionaryHeader {
  u32 filesize;
  u32 leaf_nodes_count;
};

constexpr u32 DictionaryHeaderBinSize = 8;

static DictionaryHeader ReadDictionaryHeader(std::span<const u8> data) {
  assert(data.size_bytes() >= DictionaryHeaderBinSize);
  return DictionaryHeader{.filesize = rsl::load<u32>(data, 0),
                          .leaf_nodes_count = rsl::load<u32>(data, 4)};
}

static size_t GetTotalNodeCount(const DictionaryHeader& header) {
  // The header node count does not include the root node
  return header.leaf_nodes_count + 1;
}

struct DictionaryNode {
  u16 _00;
  u16 _02;

  u16 left;
  u16 right;

  std::string_view name;
  s32 rel_data_ofs;
};

constexpr u32 DictionaryNodeBinSize = 16;

static DictionaryNode ReadDictionaryNode(std::span<const u8> whole_data,
                                         unsigned collection_offset,
                                         unsigned index) {
  const unsigned data_start = collection_offset + DictionaryHeaderBinSize +
                              DictionaryNodeBinSize * index;
  auto data = whole_data.subspan(data_start);
  assert(data.size_bytes() >= DictionaryNodeBinSize);

  return DictionaryNode{
      ._00 = rsl::load<u16>(data, 0),
      ._02 = rsl::load<u16>(data, 2),
      .left = rsl::load<u16>(data, 4),
      .right = rsl::load<u16>(data, 6),
      .name = ReadStringPointer(whole_data, data_start + 8, collection_offset),
      .rel_data_ofs = rsl::load<s32>(data, 12)};
}

static size_t CalcAbsDataOffset(const DictionaryNode& node, u32 node_offset) {
  return node_offset + node.rel_data_ofs;
}

struct SimpleDictionaryNode {
  std::string_view name;
  size_t abs_data_ofs;
};

class DictionaryRange {
public:
  DictionaryRange(std::span<const u8> whole_data, size_t start,
                  size_t total_nodes)
      : mWholeData(whole_data), mStart(start), mTotalNodes(total_nodes) {}

  struct Sentinel {};

  class ConstIter {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = SimpleDictionaryNode;
    using reference_type = SimpleDictionaryNode&;

    ConstIter(std::span<const u8> whole_data, size_t start, size_t num_nodes)
        : mWholeData(whole_data), mStart(start), mNumNodes(num_nodes) {}

    ConstIter() {}

    SimpleDictionaryNode GetSimpleNode() const {
      // Assert in ReadDictionaryNode
      auto node = ReadDictionaryNode(mWholeData, mStart, mIndex);

      return SimpleDictionaryNode{
          .name = node.name, .abs_data_ofs = CalcAbsDataOffset(node, mStart)};
    }

    SimpleDictionaryNode operator*() const { return GetSimpleNode(); }

    ConstIter& operator++() {
      ++mIndex;
      return *this;
    }

    ConstIter operator++(int) {
      return ConstIter(mWholeData, mIndex + 1, mNumNodes);
    }

    bool operator==(const ConstIter& rhs) const { return mIndex == rhs.mIndex; }
    bool operator==(const Sentinel&) const { return mIndex == mNumNodes; }

  private:
    std::span<const u8> mWholeData;
    size_t mStart;
    size_t mIndex = 1;
    size_t mNumNodes;
  };

#if __cpp_lib_concepts >= 201907L && !defined(__GNUC__)
  template <std::input_iterator T> constexpr static bool IsInputIt() {
    return true;
  }
  // static_assert(IsInputIt<ConstIter>());
#endif

  ConstIter begin() const { return ConstIter(mWholeData, mStart, mTotalNodes); }
  Sentinel end() const { return Sentinel{}; }

  size_t size() const { return mTotalNodes - 1; }

private:
  std::span<const u8> mWholeData;
  std::size_t mStart;
  std::size_t mTotalNodes;
};

static std::optional<DictionaryRange>
ReadDictionary(std::span<const u8> whole_data, size_t offset) {
  const auto data = whole_data.subspan(offset);

  if (data.size_bytes() < DictionaryHeaderBinSize) {
    return std::nullopt;
  }

  const auto header = ReadDictionaryHeader(data);
  // TODO: Validate header filesize

  const auto total_nodes = GetTotalNodeCount(header);
  const auto required_size =
      DictionaryHeaderBinSize + DictionaryNodeBinSize * total_nodes;

  if (data.size_bytes() < required_size) {
    return std::nullopt;
  }

  return DictionaryRange(whole_data, offset, total_nodes);
}

} // namespace librii::g3d
