#pragma once

#include <core/common.h>
#include <map>
#include <memory>
#include <tuple>
#include <vector>

namespace librii::glhelper {

//----------------------------------
// Vertex attribute generation
struct VAOEntry {
  u32 binding_point;
  const char* name;

  u32 format; // (gl)
  u32 size;   // (of element / stride)
};

// WIP..
struct VBOBuilder {
  VBOBuilder();
  ~VBOBuilder();

  std::vector<u8> mData;
  std::vector<u32> mIndices;

  struct VertexArray {
    VAOEntry descriptor;
    std::vector<u8> data;
  };

  // binding_point : data
  std::map<u32, VertexArray> mPropogating;

  void build();

  template <typename T> void pushData(u32 binding_point, const T& data) {
    auto& attrib_buf = mPropogating[binding_point];
    const std::size_t begin = attrib_buf.data.size();
    attrib_buf.data.resize(attrib_buf.data.size() + sizeof(T));
    *reinterpret_cast<T*>(attrib_buf.data.data() + begin) = data;
  }

  void bind();
  void unbind();
  u32 getGlId() const { return VAO; }

private:
  u32 VAO;
  u32 mPositionBuf, mIndexBuf;

private:
  template <typename T> void push(const T& data) {
    const std::size_t begin = mData.size();
    mData.resize(mData.size() + sizeof(T));
    *reinterpret_cast<T*>(mData.data() + begin) = data;
  }
};

} // namespace librii::glhelper
