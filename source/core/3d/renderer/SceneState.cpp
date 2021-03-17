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

  mTree.forEachNode([&](SceneNode& node) { node.expandBound(bound); });

  return bound;
}

void SceneState::buildUniformBuffers(const glm::mat4& view,
                                     const glm::mat4& proj) {
  mUboBuilder.clear();

  const glm::mat4 mdl{1.0f};

  mTree.forEachNode([&](SceneNode& node) {
    node.buildUniformBuffer(mUboBuilder, mdl, view, proj);
  });
}

void SceneState::draw() {
  mUboBuilder.submit();

  u32 i = 0;
  mTree.forEachNode([&](SceneNode& node) { node.draw(mUboBuilder, i++); });

  glBindVertexArray(0);
  glUseProgram(0);
}

} // namespace riistudio::lib3d
