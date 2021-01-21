#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "SceneState.hpp"
#include <core/3d/gl.hpp>
#include <plugins/j3d/Shape.hpp> // Hack
#include <vendor/glm/matrix.hpp>

namespace riistudio::lib3d {

SceneState::~SceneState() {
  for (const auto& tex : mTextures)
    glDeleteTextures(1, &tex.id);
  mTextures.clear();
}

void SceneState::buildBuffers() {
  for (const auto& node : mTree.opaque) {
    node->buildVertexBuffer(mVbo);
  }
  for (const auto& node : mTree.translucent) {
    node->buildVertexBuffer(mVbo);
  }

  mVbo.build();
  glBindVertexArray(0);
}

void SceneState::buildTextures(const lib3d::Scene& root) {
  for (const auto& tex : mTextures)
    glDeleteTextures(1, &tex.id);
  mTextures.clear();
  texIdMap.clear();

  const auto textures = root.getTextures();

  mTextures.resize(textures.size());
  std::vector<u8> data(1024 * 1024 * 4 * 2);
  for (int i = 0; i < textures.size(); ++i) {
    const auto& tex = textures[i];

    // TODO: Wrapping mode, filtering, mipmaps
    glGenTextures(1, &mTextures[i].id);
    const auto texId = mTextures[i].id;

    texIdMap[tex.getName()] = texId;
    glBindTexture(GL_TEXTURE_2D, texId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex.getMipmapCount());
    tex.decode(data, true);
    u32 slide = 0;
    for (u32 i = 0; i <= tex.getMipmapCount(); ++i) {
      glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, tex.getWidth() >> i,
                   tex.getHeight() >> i, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                   data.data() + slide);
      slide += (tex.getWidth() >> i) * (tex.getHeight() >> i) * 4;
    }
  }
}

void SceneState::gather(const lib3d::Model& model, const lib3d::Scene& texture,
                        bool buf, bool tex) {
  mTree.gather(model, texture);
  if (buf)
    buildBuffers();
  if (tex)
    buildTextures(texture);

  //	const auto mats = root.getFolder<lib3d::Material>();
  //	const auto mat = mats->at<lib3d::Material>(0);
  //	const auto compiled = mat->generateShaders();
}

void SceneState::build(const glm::mat4& view, const glm::mat4& proj,
                       riistudio::lib3d::AABB& bound) {
  // TODO
  bound.min = {0.0f, 0.0f, 0.0f};
  bound.max = {0.0f, 0.0f, 0.0f};

  for (const auto& node : mTree.opaque) {
    node->expandBound(bound);
  }
  for (const auto& node : mTree.translucent) {
    node->expandBound(bound);
  }

  // const f32 dist = glm::distance(bound.m_minBounds, bound.m_maxBounds);

  mUboBuilder.clear();
  u32 i = 0;

  glm::mat4 mdl{1.0f};

  for (const auto& node : mTree.opaque) {
    node->buildUniformBuffer(mUboBuilder, texIdMap, i++, mdl, view, proj);
  }
  for (const auto& node : mTree.translucent) {
    node->buildUniformBuffer(mUboBuilder, texIdMap, i++, mdl, view, proj);
  }

  // mUboBuilder.submit();
}

void SceneState::draw() {}

} // namespace riistudio::lib3d
