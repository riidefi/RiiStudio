#include "UBOBuilder.hpp"
#include <algorithm>
#include <core/3d/gl.hpp>
#include <cstdio>

namespace librii::glhelper {

UBOBuilder::UBOBuilder() {
#if defined(NDEBUG) || !defined(_WIN32)
  glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformStride);
  printf("UBOBuilder: Buffer offset alignment: %i\n", uniformStride);

#else
  uniformStride = 1024;
#endif

  // FIXME: Unknown bug when using native alignment offset
  uniformStride = 1024;

  glGenBuffers(1, &UBO);
}

UBOBuilder::~UBOBuilder() { glDeleteBuffers(1, &UBO); }

void DelegatedUBOBuilder::submit() {
  std::size_t size = 0;
  for (const auto& binding_point_collection : mData) {
    // printf("DelegatedUBOBuilder: Entry offset: %u\n",
    // static_cast<u32>(size));
    // The user must ensure these are aligned!
    for (const auto& binding_point_entry : binding_point_collection)
      size += binding_point_entry.size();
    size = roundUniformUp(size);
  }
  // printf("DelegatedUBOBuilder: Final size: %u\n", static_cast<u32>(size));

  // if (size > mCoalesced.size())
  mCoalesced.resize(size);
  mCoalescedOffsets.clear();

  std::size_t cursor = 0;
  for (const auto& binding_point_collection : mData) {
    if (binding_point_collection.size() < 1) {
      mCoalescedOffsets.emplace_back(cursor, 0);
      continue;
    }

    mCoalescedOffsets.emplace_back(cursor, binding_point_collection[0].size());
    // The user must ensure these are aligned!
    for (const auto& binding_point_entry : binding_point_collection) {
      assert(cursor + binding_point_entry.size() <= size);
      memcpy(mCoalesced.data() + cursor, binding_point_entry.data(),
             binding_point_entry.size());
      cursor += binding_point_entry.size();
      // printf("Binding point entry size: %u\n",
      // static_cast<u32>(binding_point_entry.size()));
    }
    cursor = roundUniformUp(cursor);
  }

  assert(cursor == mCoalesced.size() && cursor == size);

  glBindBuffer(GL_UNIFORM_BUFFER, getUboId());
  glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, size, mCoalesced.data());
}

// Use the data at each binding point
void DelegatedUBOBuilder::use(u32 idx) const {
  auto bindBufferRange = [&](GLenum target, GLuint index, GLuint buffer,
                             GLintptr offset, GLsizeiptr size) {
    assert((offset % getUniformAlignment()) == 0);

    glBindBufferRange(target, index, buffer, offset, size);
  };
  for (int i = 0; i < mData.size(); ++i) {
    const auto& ofs = mCoalescedOffsets[i];
    if (ofs.second * (mData[i].size() - idx) == 0)
      continue;
    assert(ofs.first + ofs.second * idx < mCoalesced.size());
    bindBufferRange(GL_UNIFORM_BUFFER, i, getUboId(),
                    ofs.first + ofs.second * idx,
                    ofs.second * (mData[i].size() - idx));
  }
}

void DelegatedUBOBuilder::push(u32 binding_point, const std::vector<u8>& data) {
  if (binding_point >= mData.size())
    mData.resize(binding_point + 1);
  //	else
  //	  assert(mData[binding_point].empty() ||
  //	         mData[binding_point][0].size() >= data.size());
  auto& bound_data = mData[binding_point].emplace_back(data);

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

  assert(mMinSizes[binding_point] == roundUniformUp(min));
}

} // namespace librii::glhelper
