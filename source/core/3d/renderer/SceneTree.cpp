#include "SceneTree.hpp"

#include <algorithm>
#include <core/3d/gl.hpp>
#include <glm/glm.hpp>
#include <librii/gl/Compiler.hpp>
#include <librii/gl/EnumConverter.hpp>
#include <librii/glhelper/UBOBuilder.hpp>
#include <plugins/gc/Export/Scene.hpp>

#include "SceneState.hpp"

namespace riistudio::lib3d {

void useTexObj(const TextureObj& obj) {
  glActiveTexture(GL_TEXTURE0 + obj.active_id);
  glBindTexture(GL_TEXTURE_2D, obj.image_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, obj.glMinFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, obj.glMagFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, obj.glWrapU);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, obj.glWrapV);
}

void drawReplacement(const SceneNode& node,
                     librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                     u32 draw_index) {
  librii::gl::setGlState(node.mega_state);
  glUseProgram(node.shader_id);
  glBindVertexArray(node.vao_id);
  ubo_builder.use(draw_index);
  for (auto& obj : node.texture_objects)
    useTexObj(obj);
  glDrawElements(node.glBeginMode, node.vertex_count, node.glVertexDataType,
                 node.indices);
}

} // namespace riistudio::lib3d
