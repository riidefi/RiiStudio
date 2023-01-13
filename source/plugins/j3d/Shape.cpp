#include "Model.hpp"
#include "Scene.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace riistudio::j3d {

std::span<const glm::vec2> Shape::getUv(const libcube::Model& mdl,
                                        u64 chan) const {
  auto& mMdl = reinterpret_cast<const Model&>(mdl);
  if (chan > mMdl.mBufs.uv.size())
    return {};
  return mMdl.mBufs.uv[chan].mData;
}
std::span<const glm::vec3> Shape::getPos(const libcube::Model& mdl) const {
  auto& mMdl = reinterpret_cast<const Model&>(mdl);
  return mMdl.mBufs.pos.mData;
}
std::span<const glm::vec3> Shape::getNrm(const libcube::Model& mdl) const {
  auto& mMdl = reinterpret_cast<const Model&>(mdl);
  return mMdl.mBufs.norm.mData;
}
std::span<const librii::gx::Color> Shape::getClr(const libcube::Model& mdl,
                                                 u64 chan) const {
  auto& mMdl = reinterpret_cast<const Model&>(mdl);
  if (chan > mMdl.mBufs.color.size())
    return {};
  return mMdl.mBufs.color[chan].mData;
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

} // namespace riistudio::j3d
