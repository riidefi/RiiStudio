#pragma once

#include <core/common.h>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

struct UBOBuilder {
  UBOBuilder();
  ~UBOBuilder();

  inline u32 roundUniformUp(u32 ofs) { return roundUp(ofs, uniformStride); }
  inline u32 roundUniformDown(u32 ofs) { return roundDown(ofs, uniformStride); }
  inline int getUniformAlignment() const { return uniformStride; }
  inline u32 getUboId() const { return UBO; }

private:
  int uniformStride = 0;
  u32 UBO;
};

// Prototype -- we can do this much more efficiently
struct DelegatedUBOBuilder : public UBOBuilder {
  DelegatedUBOBuilder() = default;
  ~DelegatedUBOBuilder() = default;

  MODULE_PRIVATE : void submit();

  // Use the data at each binding point
  void use(u32 idx);

  MODULE_PUBLIC : virtual void push(u32 binding_point,
                                    const std::vector<u8> &data);

  template <typename T> inline void tpush(u32 binding_point, const T &data) {
    std::vector<u8> tmp(sizeof(T));
    *reinterpret_cast<T *>(tmp.data()) = data;
    push(binding_point, tmp);
  }

  virtual void setBlockMin(u32 binding_point, u32 min);

  MODULE_PRIVATE : void clear();

private:
  // Indices as binding ids
  using RawData = std::vector<u8>;
  std::vector<std::vector<RawData>> mData;

  std::vector<u32> mMinSizes;

  // Recomputed each submit
  std::vector<u8> mCoalesced;
  std::vector<std::pair<u32, u32>> mCoalescedOffsets; // Offset : Stride
};

//----------------------------------
// Vertex attribute generation
struct VAOEntry {
  u32 binding_point;
  const char *name;

  u32 format; // (gl)
  u32 size;   // (of element / stride)
};

// WIP..
struct VBOBuilder {
  VBOBuilder();
  ~VBOBuilder();

  std::vector<u8> mData;
  std::vector<u32> mIndices;

  std::map<u32, std::pair<VAOEntry, std::vector<u8>>>
      mPropogating; // binding_point : data

  void build();

  template <typename T> void pushData(u32 binding_point, const T &data) {
    auto &attrib_buf = mPropogating[binding_point];
    const std::size_t begin = attrib_buf.second.size();
    attrib_buf.second.resize(attrib_buf.second.size() + sizeof(T));
    *reinterpret_cast<T *>(attrib_buf.second.data() + begin) = data;
  }
  u32 VAO;
  u32 mPositionBuf, mIndexBuf;

private:
  template <typename T> void push(const T &data) {
    const std::size_t begin = mData.size();
    mData.resize(mData.size() + sizeof(T));
    *reinterpret_cast<T *>(mData.data() + begin) = data;
  }
};
