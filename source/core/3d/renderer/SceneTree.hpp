#pragma once

#include <core/3d/i3dmodel.hpp>
#include <core/util/array_vector.hpp>
#include <cstring>
#include <librii/glhelper/ShaderCache.hpp>
#include <map>

namespace riistudio::lib3d {

// Not implemented yet
enum class RenderPass {
  Standard,
  ID // For selection
};

struct TextureObj {
  u32 active_id;
  u32 image_id;
  u32 glMinFilter;
  u32 glMagFilter;
  u32 glWrapU;
  u32 glWrapV;
};

void useTexObj(const TextureObj& obj);

struct SceneNode {
  virtual ~SceneNode() = default;

  // Draw the node to the screen
  // virtual void draw(librii::glhelper::DelegatedUBOBuilder& ubo_builder,
  //                   u32 draw_index) = 0;

  librii::gfx::MegaState mega_state;
  u32 shader_id;
  u32 vao_id;

  util::array_vector<TextureObj, 8> texture_objects;

  u32 glBeginMode; // GL_TRIANGLES
  u32 vertex_count;
  u32 glVertexDataType; // GL_UNSIGNED_INT
  void* indices;

  // Expand an AABB with the current bounding box
  //
  // Note: Model-space
  // virtual void expandBound(librii::math::AABB& bound) = 0;
  librii::math::AABB bound;

  // Fill-in a UBO
  //
  // Called every frame
  virtual void
  buildUniformBuffer(librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                     const glm::mat4& model_matrix,
                     const glm::mat4& view_matrix,
                     const glm::mat4& proj_matrix) = 0;
};

void drawReplacement(const SceneNode& node,
                     librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                     u32 draw_index);

struct DrawBuffer {
  std::vector<std::unique_ptr<SceneNode>> nodes;

  auto begin() { return nodes.begin(); }
  auto begin() const { return nodes.begin(); }
  auto end() { return nodes.end(); }
  auto end() const { return nodes.end(); }

  void zSort() {
    // FIXME: Implement
  }
};

struct SceneBuffers {
  using Node = SceneNode;

  DrawBuffer opaque;
  DrawBuffer translucent;

  template <typename T> void forEachNode(T functor) {
    for (auto& node : opaque)
      functor(*node);
    for (auto& node : translucent)
      functor(*node);
  }
};

} // namespace riistudio::lib3d
