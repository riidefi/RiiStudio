#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "i3dmodel.hpp"
#include <algorithm>                       // std::min
#include <core/3d/gl.hpp>                  // glClearColor
#include <core/3d/renderer/SceneState.hpp> // SceneState
#include <core/util/gui.hpp>               // ImGui::GetStyle()

#include <plugins/gc/Export/IndexedPolygon.hpp>

extern bool gTestMode;

namespace riistudio::lib3d {

glm::mat4 Bone::calcSrtMtx(const lib3d::Model* mdl) const {
  if (mdl == nullptr)
    return {};

  return calcSrtMtx(mdl->getBones());
}

SceneImpl::SceneImpl()
    : mState(gTestMode ? nullptr : std::make_shared<SceneState>()) {}

void SceneImpl::prepare(const kpi::INode& _host) {
  auto& host = *dynamic_cast<const Scene*>(&_host);

  kpi::ConstCollectionRange<Model> models = host.getModels();

  if (!models.empty())
    mState->gather(models[0], host, true, true);
}

void SceneImpl::build(const glm::mat4& view, const glm::mat4& proj,
                      AABB& bound) {
  mState->build(view, proj, bound);
}

void SceneImpl::draw() {
  glEnable(GL_DEPTH_TEST);
  MegaState state;

  auto drawNode = [&](const auto& node) {
    node->mat.setMegaState(state);

    glEnable(GL_BLEND);
    glBlendFunc(state.blendSrcFactor, state.blendDstFactor);
    glBlendEquation(state.blendMode);
    if (state.cullMode == -1) {
      glDisable(GL_CULL_FACE);
    } else {
      glEnable(GL_CULL_FACE);
      glCullFace(state.cullMode);
    }
    glFrontFace(state.frontFace);
    glDepthMask(state.depthWrite ? GL_TRUE : GL_FALSE);
    glDepthFunc(state.depthCompare);

    // assert(mState->mVbo.VAO && node->idx_size >= 0 && node->idx_size % 3 ==
    // 0);
    glUseProgram(node->shader.getId());

    glBindVertexArray(mState->mVbo.VAO);

    node->mat.genSamplUniforms(node->shader.getId(), mState->texIdMap);

    int i = 0;
    for (auto& splice :
         mState->mVbo.getSplicesInRange(node->idx_ofs, node->idx_size)) {
      assert(splice.size > 0);
      DelegatedUBOBuilder PacketBuilder;

      glUniformBlockBinding(
          node->shader.getId(),
          glGetUniformBlockIndex(node->shader.getId(), "ub_PacketParams"), 2);

      int min;
      glGetActiveUniformBlockiv(node->shader.getId(), 2,
                                GL_UNIFORM_BLOCK_DATA_SIZE, &min);
      PacketBuilder.setBlockMin(2, min);

      node->mat.onSplice(PacketBuilder, node->mdl, node->poly, i);
      // TODO: huge hack, very expensive
      PacketBuilder.submit();

      mState->mUboBuilder.use(node->mtx_id);
      PacketBuilder.use(0);
      struct PacketParams {
        glm::mat3x4 posMtx[10];
      };
      PacketParams pack{};
      for (auto& p : pack.posMtx)
        p = glm::transpose(glm::mat4{1.0f});

      assert(dynamic_cast<const libcube::IndexedPolygon*>(&node->poly) !=
             nullptr);
      const auto& ipoly =
          reinterpret_cast<const libcube::IndexedPolygon&>(node->poly);

      if (node->poly.isVisible()) {

        const auto mtx = ipoly.getPosMtx(
            reinterpret_cast<const libcube::Model&>(node->mdl), i);
        for (int p = 0; p < std::min(static_cast<std::size_t>(10), mtx.size());
             ++p) {
          const auto transposed = glm::transpose((glm::mat4x3)mtx[p]);
          pack.posMtx[p] = transposed;
        }

        if (node->poly.isVisible())
          glDrawElements(GL_TRIANGLES, splice.size, GL_UNSIGNED_INT,
                         (void*)(splice.offset * 4));
      }
      ++i;
    }
  };

  mState->mUboBuilder.submit();

  for (const auto& node : mState->mTree.opaque) {
    drawNode(node);
  }
  for (const auto& node : mState->mTree.translucent) {
    drawNode(node);
  }

  glBindVertexArray(0);
  glUseProgram(0);
  glDepthMask(GL_TRUE);
}

} // namespace riistudio::lib3d
