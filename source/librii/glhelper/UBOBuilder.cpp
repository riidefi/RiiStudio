#include "UBOBuilder.hpp"
#include <algorithm>
#include <core/3d/gl.hpp>
#include <cstdio>

namespace librii::glhelper {

#ifdef RII_GL
//
// Basic UBOBuilder
//
UBOBuilder::UBOBuilder() {
#if !defined(_WIN32)
  uniformStride = 1024; // TODO: This is really for emscripten, perhaps there is
                        // a fixed value or proper way to query this.
#else
  glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformStride);
  printf("UBOBuilder: Buffer offset alignment: %i\n", uniformStride);

  int maxBlockSize;
  glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxBlockSize);
  printf("UBOBuilder: Max block size: %i\n", maxBlockSize);
#endif

#ifdef DEBUG
  // This allows for GPU-agnostic dump analysis.
  //
  // While the alignment on Nvidia hardware is 256, on Intel hardware it is
  // usually finer. Using the hardware minimum renders Intel captures unplayable
  // on a Nvidia GPU.
  uniformStride = std::max(uniformStride, 256);
#endif

  glGenBuffers(1, &UBO);
}

UBOBuilder::~UBOBuilder() { glDeleteBuffers(1, &UBO); }

//
// Advanced UBOBuilder
//

void DelegatedUBOBuilder::submit() {
  const auto blob_size = calcBlobSize();
  mCoalesced.resize(blob_size);
  mCoalescedOffsets.clear();

  u32 cursor = 0;
  for (const auto& binding_point_collection : mData) {
    // If there is no data, add a null entry
    if (binding_point_collection.empty()) {
      mCoalescedOffsets.push_back({.offset = cursor, .stride = 0});
      continue;
    }

    // This assumes the size of each binding point entry is the same
    const std::size_t entry_size = binding_point_collection[0].size();
    const auto entry_stride = static_cast<u32>(roundUniformUp(entry_size));

    mCoalescedOffsets.push_back(
        {.offset = cursor, .stride = static_cast<u32>(entry_stride)});

    for (const auto& binding_point_entry : binding_point_collection) {
      assert(binding_point_entry.size() == entry_size &&
             "Uniforms differ in size across the binding point");

      mCoalesced.insert_bytes(cursor, binding_point_entry.data(),
                              binding_point_entry.size());
      cursor += entry_stride;
    }
  }

  assert(cursor == blob_size && cursor == mCoalesced.size());

  glBindBuffer(GL_UNIFORM_BUFFER, getUboId());
  glBufferData(GL_UNIFORM_BUFFER, blob_size, NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, blob_size, mCoalesced.data());
}

// Use the data at each binding point
void DelegatedUBOBuilder::use(u32 idx) const {
  for (int i = 0; i < mData.size(); ++i) {
    const auto& ofs = mCoalescedOffsets[i];

    const auto range_offset = ofs.offset + ofs.stride * idx;
    const int range_size = ofs.stride * (mData[i].size() - idx);

    if (range_size <= 0)
      continue;

    assert(range_offset % getUniformAlignment() == 0);
    if (range_size == 0)
      continue;

    assert(range_offset < mCoalesced.size());
    glBindBufferRange(GL_UNIFORM_BUFFER, i, getUboId(), range_offset,
                      range_size);
  }
}

void DelegatedUBOBuilder::push(u32 binding_point, const std::vector<u8>& data) {
  auto& bound_data = getTempData(binding_point).emplace_back(data);

  assert(mMinSizes.size() > binding_point);
  if (mMinSizes[binding_point] > 1024 * 1024 * 1024) {
    assert(!"Invalid minimum size. Likely a shader compilation error earlier.");
    abort();
  }
  if (bound_data.size() < mMinSizes[binding_point])
    bound_data.resize(mMinSizes[binding_point]);
}
void DelegatedUBOBuilder::clear() { mData.clear(); }

void DelegatedUBOBuilder::setBlockMin(u32 binding_point, u32 min) {
  if (binding_point >= mMinSizes.size()) {
    mMinSizes.resize(binding_point + 1);
    mMinSizes[binding_point] = roundUniformUp(min);
  }

  // assert(mMinSizes[binding_point] == roundUniformUp(min) &&
  //        "Likely shader compilation failed");
}
#endif

} // namespace librii::glhelper
