#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SceneState.hpp"
#include <core/3d/gl.hpp>
#include <plugins/j3d/Shape.hpp> // Hack
#include <vendor/glm/matrix.hpp>

namespace riistudio::lib3d {

librii::math::AABB SceneState::computeBounds() {
  librii::math::AABB bound;
  // TODO
  bound.min = {0.0f, 0.0f, 0.0f};
  bound.max = {0.0f, 0.0f, 0.0f};

  mTree.forEachNode([&](SceneNode& node) { bound.expandBound(node.bound); });

  return bound;
}

void SceneState::buildUniformBuffers() {
  mUboBuilder.clear();

  mTree.forEachNode([&](SceneNode& node) {
    node.buildUniformBuffer(mUboBuilder);
  });
}

void SceneState::draw() {
  mUboBuilder.submit();

  u32 i = 0;
  mTree.forEachNode(
      [&](SceneNode& node) { drawReplacement(node, mUboBuilder, i++); });

  glBindVertexArray(0);
  glUseProgram(0);
}

} // namespace riistudio::lib3d
