#pragma once

#include <librii/gfx/SceneNode.hpp>
#include <librii/glhelper/UBOBuilder.hpp> // DelegatedUBOBuilder
#include <librii/glhelper/VBOBuilder.hpp> // VBOBuilder
#include <librii/math/aabb.hpp>           // AABB

namespace librii::gfx {

// Not implemented yet
enum class RenderPass {
  Standard,
  ID // For selection
};

struct DrawBuffer {
  std::vector<librii::gfx::SceneNode> nodes;

  auto begin() { return nodes.begin(); }
  auto begin() const { return nodes.begin(); }
  auto end() { return nodes.end(); }
  auto end() const { return nodes.end(); }

  void zSort() {
    // FIXME: Implement
  }
};

struct SceneBuffers {
  using Node = librii::gfx::SceneNode;

  DrawBuffer opaque;
  DrawBuffer translucent;

  template <typename T> void forEachNode(T functor) {
    for (auto& node : opaque)
      functor(node);
    for (auto& node : translucent)
      functor(node);
  }
};

struct SceneState {
public:
  SceneState() = default;
  ~SceneState() = default;

  // Compute the composite bounding box (in model space)
  librii::math::AABB computeBounds();

  // Build the UBO. Typically called every frame.
  void buildUniformBuffers();

  // Draw the model to the screen. You'll want to clear it first.
  void draw();

  // Direct access to attached renderables.
  SceneBuffers& getBuffers() { return mTree; }

  void invalidate() {
    mTree.opaque.nodes.clear();
    mTree.translucent.nodes.clear();
  }

private:
  SceneBuffers mTree;
  std::vector<bool> mUploaded;
  librii::glhelper::DelegatedUBOBuilder mUboBuilder;
};

} // namespace librii::gfx
