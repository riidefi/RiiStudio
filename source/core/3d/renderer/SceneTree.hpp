#pragma once

#include <core/3d/i3dmodel.hpp>
#include <librii/glhelper/ShaderCache.hpp>
#include <map>

namespace riistudio::lib3d {

// Not implemented yet
enum class RenderPass {
  Standard,
  ID // For selection
};

struct SceneNode : public lib3d::IObserver {
  virtual ~SceneNode() = default;

  // Draw the node to the screen
  virtual void draw(librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                    u32 draw_index) = 0;

  // Expand an AABB with the current bounding box
  //
  // Note: Model-space
  virtual void expandBound(AABB& bound) = 0;

  // Fill-in a UBO
  //
  // Called every frame
  virtual void
  buildUniformBuffer(librii::glhelper::DelegatedUBOBuilder& ubo_builder,
                     const glm::mat4& model_matrix,
                     const glm::mat4& view_matrix,
                     const glm::mat4& proj_matrix) = 0;
};

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
