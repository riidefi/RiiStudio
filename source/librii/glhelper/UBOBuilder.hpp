#pragma once

#include <core/common.h>
#include <map>
#include <memory>
#include <string.h>
#include <tuple>
#include <vector>

namespace librii::glhelper {

struct UBOBuilder {
  UBOBuilder();
  ~UBOBuilder();

  u32 roundUniformUp(u32 ofs) const {
    auto res = roundUp(ofs, uniformStride);
    assert((res % uniformStride) == 0);
    assert(res >= ofs);
    return res;
  }
  u32 roundUniformDown(u32 ofs) const {
    auto res = roundDown(ofs, uniformStride);
    assert((res % uniformStride) == 0);
    assert(res <= ofs);
    return res;
  }
  int getUniformAlignment() const { return uniformStride; }
  u32 getUboId() const { return UBO; }

private:
  int uniformStride = 0;
  u32 UBO;
};

// A write-only binary object.
class Blob {
public:
  Blob() = default;
  Blob(std::vector<u8>&& data) : mData(std::move(data)) {}
  Blob(const std::vector<u8>& data) : mData(data) {}
  Blob(Blob&& blob) : Blob(std::move(blob.mData)) {}

  std::size_t size() const { return mData.size(); }
  void resize(std::size_t size) { mData.resize(size); }
  void clear() { mData.clear(); }

  const u8* data() const { return mData.data(); }

  void insert_bytes(std::size_t begin, const u8* bytes,
                    std::size_t byte_count) {
    if (begin + byte_count > mData.size()) {
      assert(!"insert_bytes: range is out of bounds");
      return;
    }

    memcpy(mData.data() + begin, bytes, byte_count);
  }

private:
  std::vector<u8> mData;
};

// Prototype -- we can do this much more efficiently
class DelegatedUBOBuilder : public UBOBuilder {
public:
  DelegatedUBOBuilder() = default;
  ~DelegatedUBOBuilder() = default;

  void submit();

  // Use the data at each binding point
  void use(u32 idx) const;

  [[nodiscard]] Result<void> push(u32 binding_point,
                                  const std::vector<u8>& data);

  template <typename T>
  [[nodiscard]] Result<void> tpush(u32 binding_point, const T& data) {
    std::vector<u8> tmp(sizeof(T));
    *reinterpret_cast<T*>(tmp.data()) = data;
    return push(binding_point, tmp);
  }
  void reset(u32 binding_point) {
    // Check if the binding point has been set
    if (mData.size() > binding_point)
      return;

    if (!mData[binding_point].empty()) {
      mData[binding_point][mData[binding_point].size() - 1].clear();
    }
  }

  void setBlockMin(u32 binding_point, u32 min);

  void clear();

private:
  // Indices as binding ids
  std::vector<std::vector<Blob>> mData;

  std::vector<u32> mMinSizes;

  // Recomputed each submit
  Blob mCoalesced;
  struct CoalescedEntry {
    u32 offset = 0;
    u32 stride = 0;
  };
  std::vector<CoalescedEntry> mCoalescedOffsets;

  std::vector<Blob>& getTempData(u32 binding_point) {
    while (binding_point >= mData.size())
      mData.emplace_back();

    return mData[binding_point];
  }

  std::size_t calcBlobSize() const {
    std::size_t size = 0;
    for (const auto& binding_point_collection : mData) {
      for (const auto& binding_point_entry : binding_point_collection)
        size += roundUniformUp(binding_point_entry.size());
    }
    // printf("DelegatedUBOBuilder: Final size: %u\n", static_cast<u32>(size));
    return size;
  }
};

} // namespace librii::glhelper
