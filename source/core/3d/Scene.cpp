#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "i3dmodel.hpp"

#include <librii/g3d/gfx/G3dGfx.hpp>

namespace riistudio::lib3d {

// PUBLIC FUNCTION
glm::mat4 calcSrtMtx(const lib3d::Bone& bone, const lib3d::Model* mdl) {
  if (mdl == nullptr)
    return {};

  return calcSrtMtx(bone, mdl->getBones());
}

// Calculate the bounding box of a polygon
librii::math::AABB CalcPolyBound(const lib3d::Polygon& poly,
                                 const lib3d::Bone& bone,
                                 const lib3d::Model& mdl) {
  auto mdl_mtx = lib3d::calcSrtMtx(bone, &mdl);

  librii::math::AABB bound = poly.getBounds();
  auto nmax = mdl_mtx * glm::vec4(bound.max, 1.0f);
  auto nmin = mdl_mtx * glm::vec4(bound.min, 1.0f);

  return {nmin, nmax};
}

struct SceneImpl::Internal {
  librii::g3d::gfx::G3dSceneRenderData render_data;
};

SceneImpl::SceneImpl() = default;
SceneImpl::~SceneImpl() = default;

Result<void> SceneImpl::prepare(SceneState& state,
                                                    const kpi::INode& _host,
                                                    glm::mat4 v_mtx,
                                                    glm::mat4 p_mtx) {
  auto& host = *dynamic_cast<const Scene*>(&_host);

  if (mImpl == nullptr) {
    auto impl = std::make_unique<Internal>();
    TRY(impl->render_data.init(host));
    mImpl = std::move(impl);
  }

  return librii::g3d::gfx::Any3DSceneAddNodesToBuffer(state, host, v_mtx, p_mtx,
                                                      mImpl->render_data);
}

} // namespace riistudio::lib3d
