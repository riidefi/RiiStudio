#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SceneState.hpp"
#include <core/3d/gl.hpp>
#include <plugins/j3d/Shape.hpp> // Hack
#include <vendor/glm/matrix.hpp>

namespace riistudio::lib3d {

void SceneState::addTexture(const std::string& key, const lib3d::Texture& tex) {
  texIdMap[key] = mTextures.emplace_back(tex).getGlId();
}

AABB SceneState::computeBounds() {
  AABB bound;
  // TODO
  bound.min = {0.0f, 0.0f, 0.0f};
  bound.max = {0.0f, 0.0f, 0.0f};

  mTree.forEachNode([&](SceneNode& node) { node.expandBound(bound); });

  // const f32 dist = glm::distance(bound.m_minBounds, bound.m_maxBounds);
  return bound;
}

void SceneState::buildUniformBuffers(const glm::mat4& view,
                                     const glm::mat4& proj) {
  mUboBuilder.clear();
  u32 i = 0;

  const glm::mat4 mdl{1.0f};

  mTree.forEachNode([&](SceneNode& node) {
    node.buildUniformBuffer(mUboBuilder, texIdMap, i++, mdl, view, proj);
  });

  // mUboBuilder.submit();
}

void SceneState::draw() {
  mUboBuilder.submit();

  mTree.forEachNode([&](SceneNode& node) { node.draw(mUboBuilder, texIdMap); });

  glBindVertexArray(0);
  glUseProgram(0);
}

} // namespace riistudio::lib3d
