#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "i3dmodel.hpp"
#include <algorithm>                       // std::min
#include <core/3d/gl.hpp>                  // glClearColor
#include <core/3d/renderer/SceneState.hpp> // SceneState
#include <core/util/gui.hpp>               // ImGui::GetStyle()
#include <librii/gl/Compiler.hpp>          // PacketParams
#include <librii/gl/EnumConverter.hpp>     // setGlState
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
  mState->mUboBuilder.submit();

  for (const auto& node : mState->mTree.opaque) {
    node->draw(mState->mVbo, mState->mUboBuilder, mState->texIdMap);
  }
  for (const auto& node : mState->mTree.translucent) {
    node->draw(mState->mVbo, mState->mUboBuilder, mState->texIdMap);
  }

  glBindVertexArray(0);
  glUseProgram(0);
}

} // namespace riistudio::lib3d
