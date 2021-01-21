#include "VBOBuilder.hpp"
#include <algorithm>
#include <core/3d/gl.hpp>

namespace librii::glhelper {

VBOBuilder::VBOBuilder() {
  glGenBuffers(1, &mPositionBuf);
  glGenBuffers(1, &mIndexBuf);

  glGenVertexArrays(1, &VAO);

  splicePoints.push_back({});
}
VBOBuilder::~VBOBuilder() {
  glDeleteBuffers(1, &mPositionBuf);
  glDeleteBuffers(1, &mIndexBuf);

  glDeleteVertexArrays(1, &VAO);
}
std::vector<VBOBuilder::SplicePoint>
VBOBuilder::getSplicesInRange(std::size_t start, std::size_t ofs) {
  std::vector<SplicePoint> out;

  const auto min = start;
  const auto max = start + ofs;

  for (auto& s : splicePoints) {
    if (s.offset >= max)
      continue;
    if (s.offset < min)
      continue;
    out.push_back(s);
  }
  return out;
}
void VBOBuilder::build() {
  std::vector<std::pair<VAOEntry, u32>> mAttribStack; // desc : offset

  for (const auto& bind : mPropogating) {
    mAttribStack.emplace_back(bind.second.first, mData.size());

    for (const u8 e : bind.second.second)
      push(e);
  }

  glBindVertexArray(VAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuf);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * 4, mIndices.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, mPositionBuf);
  glBufferData(GL_ARRAY_BUFFER, mData.size(), mData.data(), GL_STATIC_DRAW);
  glBindVertexArray(VAO);

  auto vertexAttribPointer = [&](GLuint index, GLint size, GLenum type,
                                 GLboolean normalized, GLsizei stride,
                                 const void* pointer) {
    DebugReport("Index: %u, size: %i, stride: %i, ofs: %u\n", index, size,
                stride, (u32)pointer);

    assert(stride != 0);

    glVertexAttribPointer(index, size, type, normalized, stride, pointer);

    assert(glGetError() == GL_NO_ERROR);
    if (glGetError() != GL_NO_ERROR)
      exit(1);
  };

  for (const auto& attrib : mAttribStack) {
    // TODO: Hack
    if (attrib.first.name == nullptr)
      continue;
    assert(attrib.first.format == GL_FLOAT);
    vertexAttribPointer(attrib.first.binding_point, attrib.first.size / 4,
                        GL_FLOAT, GL_FALSE, attrib.first.size,
                        reinterpret_cast<void*>(attrib.second));
    // glVertexAttribPointer(attrib.first.binding_point, attrib.first.size,
    // attrib.first.format, GL_FALSE, attrib.first.size, (void*)attrib.second);
    glEnableVertexAttribArray(attrib.first.binding_point);
  }

  mPropogating.clear();
}

} // namespace librii::glhelper
