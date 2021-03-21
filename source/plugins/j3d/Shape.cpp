#include "Model.hpp"
#include "Scene.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace riistudio::j3d {

glm::vec2 Shape::getUv(const libcube::Model& mdl, u64 chan, u64 idx) const {
  auto& mMdl = reinterpret_cast<const Model&>(mdl);

  if (idx > mMdl.mBufs.uv[chan].mData.size())
    return {};
  return mMdl.mBufs.uv[chan].mData[idx];
}
glm::vec3 Shape::getPos(const libcube::Model& mdl, u64 idx) const {
  auto& mMdl = reinterpret_cast<const Model&>(mdl);
  return mMdl.mBufs.pos.mData[idx];
}
glm::vec3 Shape::getNrm(const libcube::Model& mdl, u64 idx) const {
  auto& mMdl = reinterpret_cast<const Model&>(mdl);
  return mMdl.mBufs.norm.mData[idx];
}
glm::vec4 Shape::getClr(const libcube::Model& mdl, u64 chan, u64 idx) const {
  auto& mMdl = reinterpret_cast<const Model&>(mdl);
  if (idx >= mMdl.mBufs.color[chan].mData.size())
    return {1, 1, 1, 1};

  auto raw = static_cast<librii::gx::ColorF32>(
      (librii::gx::Color)mMdl.mBufs.color[chan].mData[idx]);

  return {raw.r, raw.g, raw.b, raw.a};
}

template <typename X, typename Y>
auto add_to_buffer(const X& entry, Y& buf) -> u16 {
  const auto found = std::find(buf.begin(), buf.end(), entry);
  if (found != buf.end()) {
    return found - buf.begin();
  }

  buf.push_back(entry);
  return buf.size() - 1;
};

u64 Shape::addPos(libcube::Model& mdl, const glm::vec3& v) {
  return add_to_buffer(v, reinterpret_cast<Model&>(mdl).mBufs.pos.mData);
}
u64 Shape::addNrm(libcube::Model& mdl, const glm::vec3& v) {
  return add_to_buffer(v, reinterpret_cast<Model&>(mdl).mBufs.norm.mData);
}
u64 Shape::addClr(libcube::Model& mdl, u64 chan, const glm::vec4& v) {
  librii::gx::ColorF32 fclr;
  fclr.r = v[0];
  fclr.g = v[1];
  fclr.b = v[2];
  fclr.a = v[3];
  librii::gx::Color c = fclr;
  return add_to_buffer(c,
                       reinterpret_cast<Model&>(mdl).mBufs.color[chan].mData);
}
u64 Shape::addUv(libcube::Model& mdl, u64 chan, const glm::vec2& v) {
  return add_to_buffer(v, reinterpret_cast<Model&>(mdl).mBufs.uv[chan].mData);
}

glm::mat4 computeBoneMdl(u32 id, kpi::ConstCollectionRange<lib3d::Bone> bones) {
  glm::mat4 mdl(1.0f);

  auto& bone = bones[id];
  const auto parent = bone.getBoneParent();
  if (parent >= 0 && parent != id)
    mdl = computeBoneMdl(parent, bones);

  return mdl * librii::math::calcXform(bone.getSRT());
}
std::vector<glm::mat4> Shape::getPosMtx(const libcube::Model& _mdl,
                                        u64 mpid) const {
  std::vector<glm::mat4> out;

  const auto& mp = mMatrixPrimitives[mpid];

  auto& mdl = reinterpret_cast<const Model&>(_mdl);
  // if (!(getVcd().mBitfield &
  //       (1 << (int)librii::gx::VertexAttribute::PositionNormalMatrixIndex)))
  //       {
  //   return {
  //       mdl_ac.getJoint(0 /* DEBUG
  //       */).get().calcSrtMtx(&mdl_ac.getJoints())};
  // }
  for (const auto it : mp.mDrawMatrixIndices) {
    const auto& drw = mdl.mDrawMatrices[it];
    glm::mat4x4 curMtx(1.0f);

    // Rigid -- bone space
    if (drw.mWeights.size() == 1) {
      u32 boneID = drw.mWeights[0].boneId;
      curMtx = mdl.getBones()[boneID].calcSrtMtx(mdl.getBones().toConst());
    } else {
      // curMtx = glm::mat4{ 0.0f };
      // for (const auto& w : drw.mWeights) {
      //  const auto bindMtx = computeBoneMdl(w.boneId, &mdl_ac.getJoints());
      //  auto wInv = glm::inverse(bindMtx) * w.weight;
      //  curMtx = curMtx + wInv;
      //}
    }
    out.push_back(curMtx);
  }

  return out;
}

} // namespace riistudio::j3d
