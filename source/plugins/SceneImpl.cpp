#include "SceneImpl.hpp"

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

Result<void> SceneImpl::prepare(SceneState& state, const Scene& host,
                                glm::mat4 v_mtx, glm::mat4 p_mtx) {
  if (auto* lc = dynamic_cast<const libcube::Scene*>(&host)) {
    if (!uploaded) {
      uploaded = true;
      TRY(render_data.init(*lc));
    }
    return librii::g3d::gfx::Any3DSceneAddNodesToBuffer(state, *lc, v_mtx,
                                                        p_mtx, render_data);
  } else {
    return std::unexpected(
        "Cannot render scn; does not inherit from libcube::Scene");
  }
}

} // namespace riistudio::lib3d
