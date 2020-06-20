#define NOMINMAX

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
  auto setupNode = [&](auto& node) {
    node->idx_ofs = static_cast<u32>(mVbo.mIndices.size());
    node->poly.propogate(mVbo);
    node->idx_size = static_cast<u32>(mVbo.mIndices.size()) - node->idx_ofs;
  };
  // TODO -- nodes may be of incompatible type..
  for (const auto& node : mTree.opaque) {
    setupNode(node);
  }
  for (const auto& node : mTree.translucent) {
    setupNode(node);
  }

  mVbo.build();
  glBindVertexArray(0);
}

void SceneState::buildTextures(const kpi::IDocumentNode& root) {
  for (const auto& tex : mTextures)
    glDeleteTextures(1, &tex.id);
  mTextures.clear();
  texIdMap.clear();

  if (root.getFolder<lib3d::Texture>() == nullptr)
    return;

  const auto* textures = root.getFolder<lib3d::Texture>();

  mTextures.resize(textures->size());
  std::vector<u8> data(1024 * 1024 * 4 * 2);
  for (int i = 0; i < textures->size(); ++i) {
    const auto& tex = textures->at<lib3d::Texture>(i);

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

void SceneState::gather(const kpi::IDocumentNode& model,
                        const kpi::IDocumentNode& texture, bool buf, bool tex) {
  bones = model.getFolder<lib3d::Bone>();
  mTree.gather(model);
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
    auto mdl = ((lib3d::Bone&)node->bone).calcSrtMtx();

    auto nmax = mdl * glm::vec4(node->poly.getBounds().max, 0.0f);
    auto nmin = mdl * glm::vec4(node->poly.getBounds().min, 0.0f);

    riistudio::lib3d::AABB newBound{nmin, nmax};
    bound.expandBound(newBound);
  }
  for (const auto& node : mTree.translucent) {
    auto mdl = ((lib3d::Bone&)node->bone).calcSrtMtx();

    auto nmax = mdl * glm::vec4(node->poly.getBounds().max, 0.0f);
    auto nmin = mdl * glm::vec4(node->poly.getBounds().min, 0.0f);

    riistudio::lib3d::AABB newBound{nmin, nmax};
    bound.expandBound(newBound);
  }

  // const f32 dist = glm::distance(bound.m_minBounds, bound.m_maxBounds);

  mUboBuilder.clear();
  u32 i = 0;

  auto propNode = [&](const auto& node, u32 mtx_id) {
    glm::mat4 mdl(1.0f);
    // no more mdl here, we just use PNMTX0
    // mdl = computeBoneMdl(node->bone);
    // (Up to the mat)
    // if (!node->poly.hasAttrib(lib3d::Polygon::SimpleAttrib::EnvelopeIndex))
    // {
    //  mdl = ((lib3d::Bone&)node->bone).calcSrtMtx();
    //}

    node->mat.generateUniforms(mUboBuilder, mdl, view, proj,
                               node->shader.getId(), texIdMap, node->poly);
    node->mtx_id = mtx_id;
  };

  for (const auto& node : mTree.opaque) {
    propNode(node, i++);
  }
  for (const auto& node : mTree.translucent) {
    propNode(node, i++);
  }

  // mUboBuilder.submit();
}

void SceneState::draw() {}

} // namespace riistudio::lib3d
