#pragma once

#include <core/3d/i3dmodel.hpp>
#include <cstring>
#include <librii/glhelper/ShaderCache.hpp>
#include <librii/glhelper/UBOBuilder.hpp>
#include <llvm/ADT/SmallVector.h>
#include <map>
#include <rsl/ArrayVector.hpp>

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

  rsl::array_vector<TextureObj, 8> texture_objects;

  u32 glBeginMode; // GL_TRIANGLES
  u32 vertex_count;
  u32 glVertexDataType; // GL_UNSIGNED_INT
  void* indices;

  // Expand an AABB with the current bounding box
  //
  // Note: Model-space
  // virtual void expandBound(librii::math::AABB& bound) = 0;
  librii::math::AABB bound;

  struct UniformData {
    //! Binding pointer to insert the data at
    u32 binding_point;

    //! Raw data to store there
    //! Note: sizeof(UniformMaterialParams) == 1536
    llvm::SmallVector<u8, 2048> raw_data;
  };

  llvm::SmallVector<UniformData, 4> uniform_data;

  // Fill-in a UBO
  //
  // Called every frame
  virtual void
  buildUniformBuffer(librii::glhelper::DelegatedUBOBuilder& ubo_builder) = 0;
};

void drawReplacement(const SceneNode& node,
                     librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                     u32 draw_index);

inline void buildUniformBufferRepalcement(
    SceneNode& node, librii::glhelper::DelegatedUBOBuilder& ubo_builder) {
  node.buildUniformBuffer(ubo_builder);

  for (auto& command : node.uniform_data) {
    // TODO: Remove this bloat
    static std::vector<u8> scratch;
    scratch.resize(command.raw_data.size());
    memcpy(scratch.data(), command.raw_data.data(), scratch.size());

    ubo_builder.push(command.binding_point, scratch);
  }
}

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
