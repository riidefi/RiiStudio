#include "SceneNode.hpp"
#include <core/3d/gl.hpp>
#include <librii/gl/EnumConverter.hpp>

namespace librii::gfx {

#define RII_UNREACHABLE(...)                                                   \
  do {                                                                         \
    assert(!"Unreachable");                                                    \
    abort();                                                                   \
  } while (0);

u32 TranslateDataType(DataType d) {
  switch (d) {
  case DataType::Byte:
    return GL_BYTE;
  case DataType::UnsignedByte:
    return GL_UNSIGNED_BYTE;
  case DataType::Short:
    return GL_SHORT;
  case DataType::UnsignedShort:
    return GL_UNSIGNED_SHORT;
  case DataType::Int:
    return GL_INT;
  case DataType::UnsignedInt:
    return GL_UNSIGNED_INT;
  case DataType::Float:
    return GL_FLOAT;
  case DataType::Double:
    return GL_DOUBLE;
  }

  RII_UNREACHABLE();
}

u32 TranslateBeginMode(PrimitiveType t) {
  switch (t) {
  case PrimitiveType::Triangles:
    return GL_TRIANGLES;
  }

  RII_UNREACHABLE();
}

void DrawSceneNode(const librii::gfx::SceneNode& node,
                   librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                   u32 draw_index) {
#ifdef RII_GL
  librii::gl::setGlState(node.mega_state);
  glUseProgram(node.shader_id);
  glBindVertexArray(node.vao_id);
  ubo_builder.use(draw_index);
  for (auto& obj : node.texture_objects)
    librii::gfx::UseTexObj(obj);
  glDrawElements(TranslateBeginMode(node.primitive_type), node.vertex_count,
                 TranslateDataType(node.vertex_data_type), node.indices);
#endif
}

void AddSceneNodeToUBO(librii::gfx::SceneNode& node,
                       librii::glhelper::DelegatedUBOBuilder& ubo_builder) {
  for (auto& command : node.uniform_mins) {
    ubo_builder.setBlockMin(command.binding_point, command.min_size);
  }

  for (auto& command : node.uniform_data) {
    // TODO: Remove this bloat
    static std::vector<u8> scratch;
    scratch.resize(command.raw_data.size());
    memcpy(scratch.data(), command.raw_data.data(), scratch.size());

    ubo_builder.push(command.binding_point, scratch);
  }
}

} // namespace librii::gfx
