#include "SceneNode.hpp"
#include <core/3d/gl.hpp>
#include <librii/gl/EnumConverter.hpp>

namespace librii::gfx {

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
  glDrawElements(node.glBeginMode, node.vertex_count, node.glVertexDataType,
                 node.indices);
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
