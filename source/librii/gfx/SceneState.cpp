#include "SceneState.hpp"
#include <core/3d/gl.hpp>
#include <vendor/glm/matrix.hpp>

namespace riistudio::lib3d {

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
      fprintf(stderr, "ubo_builder.push error: %s\n", ok.error().c_str());
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
    librii::gfx::DrawSceneNode(node, mUboBuilder, i - 1);
  });

#ifdef RII_GL
  glBindVertexArray(0);
  glUseProgram(0);
#endif
}

} // namespace riistudio::lib3d
