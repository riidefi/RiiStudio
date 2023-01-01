#pragma once

#include <core/3d/renderer/SceneTree.hpp> // SceneBuffers
#include <librii/glhelper/UBOBuilder.hpp> // DelegatedUBOBuilder
#include <librii/glhelper/VBOBuilder.hpp> // VBOBuilder
#include <librii/math/aabb.hpp>           // AABB

namespace riistudio::lib3d {

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

} // namespace riistudio::lib3d
