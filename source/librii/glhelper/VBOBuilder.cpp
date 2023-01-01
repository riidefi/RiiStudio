#include "VBOBuilder.hpp"
#include <core/3d/gl.hpp>

namespace librii::glhelper {

#ifdef RII_GL
VBOBuilder::VBOBuilder() {
  glGenBuffers(1, &mPositionBuf);
  glGenBuffers(1, &mIndexBuf);

  glGenVertexArrays(1, &VAO);
}
VBOBuilder::~VBOBuilder() {
  glDeleteBuffers(1, &mPositionBuf);
  glDeleteBuffers(1, &mIndexBuf);

  glDeleteVertexArrays(1, &VAO);
}
Result<void> VBOBuilder::build() {
  std::vector<std::pair<VAOEntry, u32>> mAttribStack; // desc : offset

  for (const auto& bind : mPropogating) {
    mAttribStack.emplace_back(bind.second.descriptor, mData.size());

    for (const u8 e : bind.second.data)
      push(e);
  }

  uploadIndexBuffer();

  glBindBuffer(GL_ARRAY_BUFFER, mPositionBuf);
  glBufferData(GL_ARRAY_BUFFER, mData.size(), mData.data(), GL_STATIC_DRAW);
  glBindVertexArray(VAO);

  auto vertexAttribPointer = [&](GLuint index, GLint size, GLenum type,
                                 GLboolean normalized, GLsizei stride,
                                 const void* pointer) -> Result<void> {
    DebugReport("Index: %u, size: %i, stride: %i, ofs: %p\n", index, size,
                stride, pointer);

    assert(stride != 0);

    glVertexAttribPointer(index, size, type, normalized, stride, pointer);

    std::string errors;
    while (glGetError() != GL_NO_ERROR) {
      errors += std::format("{}, ", glGetError());
    }
    if (errors.size() > 2) {
      errors.resize(errors.size() - 2);
      return std::unexpected(std::format("OpenGL Errors: {}", errors));
    }
    return {};
  };

  for (const auto& attrib : mAttribStack) {
    // TODO: Hack
    if (attrib.first.name == nullptr)
      continue;

    TRY(vertexAttribPointer(
        attrib.first.binding_point, attrib.first.size / 4, GL_FLOAT, GL_FALSE,
        attrib.first.size,
        reinterpret_cast<void*>(static_cast<std::uintptr_t>(attrib.second))));
    // glVertexAttribPointer(attrib.first.binding_point, attrib.first.size,
    // attrib.first.format, GL_FALSE, attrib.first.size, (void*)attrib.second);
    glEnableVertexAttribArray(attrib.first.binding_point);
  }

  // mPropogating.clear();
  glBindVertexArray(0);
  return {};
}

void VBOBuilder::uploadIndexBuffer() {
  glBindVertexArray(VAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuf);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * 4, mIndices.data(),
               GL_DYNAMIC_DRAW /* GL_STATIC_DRAW */);
}

void VBOBuilder::bind() { glBindVertexArray(VAO); }
void VBOBuilder::unbind() { glBindVertexArray(0); }
#endif

} // namespace librii::glhelper
