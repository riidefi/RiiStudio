#include "SceneState.hpp"
#include <core/3d/gl.hpp>
#include <vendor/glm/matrix.hpp>

namespace librii::gfx {

librii::math::AABB SceneState::computeBounds() {
  librii::math::AABB bound;
  // TODO
  bound.min = {0.0f, 0.0f, 0.0f};
  bound.max = {0.0f, 0.0f, 0.0f};

  mTree.forEachNode(
      [&](librii::gfx::SceneNode& node) { bound.expandBound(node.bound); });

  return bound;
}

void SceneState::buildUniformBuffers() {
  mUboBuilder.clear();
  mUploaded.clear();
  mTree.forEachNode([&](librii::gfx::SceneNode& node) {
    auto ok = librii::gfx::AddSceneNodeToUBO(node, mUboBuilder);
    if (!ok) {
      rsl::error("ubo_builder.push error: {}", ok.error());
      mUploaded.push_back(false);
    } else {
      mUploaded.push_back(true);
    }
  });
}

void SceneState::draw() {
  mUboBuilder.submit();

  u32 i = 0;
  mTree.forEachNode([&](librii::gfx::SceneNode& node) {
    if (!mUploaded[i++])
      return;
    auto ok = librii::gfx::DrawSceneNode(node, mUboBuilder, i - 1);
    if (!ok) {
      rsl::error("DrawSceneNode failed: {}", ok.error());
    }
  });

#ifdef RII_GL
  glBindVertexArray(0);
  glUseProgram(0);
#endif
}

} // namespace librii::gfx
