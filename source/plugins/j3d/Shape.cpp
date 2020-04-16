#include "Model.hpp"
#include "Scene.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace riistudio::j3d {

const Model* getModel(const Shape* shp) {
  assert(shp->getParent());
  return dynamic_cast<const Model*>(shp->getParent());
}

glm::vec2 Shape::getUv(u64 chan, u64 idx) const {
  auto& mMdl = *getModel(this);

  if (idx > mMdl.mBufs.uv[chan].mData.size())
    return {};
  return mMdl.mBufs.uv[chan].mData[idx];
}
glm::vec3 Shape::getPos(u64 idx) const {
  auto& mMdl = *getModel(this);
  return mMdl.mBufs.pos.mData[idx];
}
glm::vec3 Shape::getNrm(u64 idx) const {
  auto& mMdl = *getModel(this);
  return mMdl.mBufs.norm.mData[idx];
}
glm::vec4 Shape::getClr(u64 idx) const {
  auto& mMdl = *getModel(this);
  if (idx >= mMdl.mBufs.color[0].mData.size())
    return {1, 1, 1, 1};

  auto raw = static_cast<libcube::gx::ColorF32>(
      (libcube::gx::Color)mMdl.mBufs.color[0].mData[idx]);

  return {raw.r, raw.g, raw.b, raw.a};
}
template <typename T>
static u64 addUnique(std::vector<T>& array, const T& value) {
  //	const auto found = std::find(array.begin(), array.end(), value);
  //	if (found != array.end())
  //		return found - array.begin();
  const auto idx = array.size();
  array.push_back(value);
  return idx;
}

u64 Shape::addPos(const glm::vec3& v) {
  return 0;
  //	auto& mMdl = *getModel(this);
  //	u64 out = addUnique(mMdl.mBufs.pos.mData, v);
  //	return out;
}
u64 Shape::addNrm(const glm::vec3& v) {
  return 0;
  // auto& mMdl = *getModel();
  // return addUnique(mMdl.mBufs.norm.mData, v);
}
u64 Shape::addClr(u64 chan, const glm::vec4& v) {
  return 0;

  //	auto& mMdl = *getModel();
  //	// TODO: Round
  //	return addUnique(mMdl.mBufs.color[chan].mData, gx::Color{
  //		static_cast<u8>(v[0] * 255.0f),
  //		static_cast<u8>(v[1] * 255.0f),
  //		static_cast<u8>(v[2] * 255.0f),
  //		static_cast<u8>(v[3] * 255.0f) });
}
u64 Shape::addUv(u64 chan, const glm::vec2& v) {
  return 0;

  //	auto& mMdl = *getModel();
  //	return addUnique(mMdl.mBufs.uv[chan].mData, v);
}

void Shape::addTriangle(std::array<SimpleVertex, 3> tri) {
  if (mMatrixPrimitives.empty())
    mMatrixPrimitives.emplace_back();
  if (mMatrixPrimitives.back().mPrimitives.empty() ||
      mMatrixPrimitives.back().mPrimitives.back().mType !=
          libcube::gx::PrimitiveType::Triangles)
    mMatrixPrimitives.back().mPrimitives.emplace_back(
        libcube::gx::PrimitiveType::Triangles, 0);

  auto& prim = mMatrixPrimitives.back().mPrimitives.back();

  for (const auto& vtx : tri) {
    // assert(!hasAttrib(SimpleAttrib::EnvelopeIndex));

    libcube::IndexedVertex ivtx;

    assert(hasAttrib(SimpleAttrib::Position));

    ivtx[libcube::gx::VertexAttribute::Position] = addPos(vtx.position);

    if (vtx.position.x > bbox.max.x)
      bbox.max.x = vtx.position.x;
    if (vtx.position.y > bbox.max.y)
      bbox.max.y = vtx.position.y;
    if (vtx.position.z > bbox.max.z)
      bbox.max.z = vtx.position.z;

    if (vtx.position.x < bbox.min.x)
      bbox.min.x = vtx.position.x;
    if (vtx.position.y < bbox.min.y)
      bbox.min.y = vtx.position.y;
    if (vtx.position.z < bbox.min.z)
      bbox.min.z = vtx.position.z;
    if (hasAttrib(SimpleAttrib::Normal))
      ivtx[libcube::gx::VertexAttribute::Normal] = addNrm(vtx.normal);
    if (hasAttrib(SimpleAttrib::Color0))
      ivtx[libcube::gx::VertexAttribute::Color0] = addClr(0, vtx.colors[0]);
    if (hasAttrib(SimpleAttrib::Color1))
      ivtx[libcube::gx::VertexAttribute::Color1] = addClr(1, vtx.colors[1]);
    // TODO: Support all
    if (hasAttrib(SimpleAttrib::TexCoord0))
      ivtx[libcube::gx::VertexAttribute::TexCoord0] = addUv(0, vtx.uvs[0]);
    if (hasAttrib(SimpleAttrib::TexCoord1))
      ivtx[libcube::gx::VertexAttribute::TexCoord1] = addUv(1, vtx.uvs[1]);

    prim.mVertices.push_back(ivtx);
  }
}

glm::mat4 computeMdlMtx(const lib3d::SRT3& srt) {
  glm::mat4 dst(1.0f);

  //	dst = glm::translate(dst, srt.translation);
  //	dst = dst * glm::eulerAngleZYX(glm::radians(srt.rotation.x),
  //glm::radians(srt.rotation.y), glm::radians(srt.rotation.z)); 	return
  //glm::scale(dst, srt.scale);

  float sinX = sin(glm::radians(srt.rotation.x)),
        cosX = cos(glm::radians(srt.rotation.x));
  float sinY = sin(glm::radians(srt.rotation.y)),
        cosY = cos(glm::radians(srt.rotation.y));
  float sinZ = sin(glm::radians(srt.rotation.z)),
        cosZ = cos(glm::radians(srt.rotation.z));

  dst[0][0] = srt.scale.x * (cosY * cosZ);
  dst[0][1] = srt.scale.x * (sinZ * cosY);
  dst[0][2] = srt.scale.x * (-sinY);
  dst[0][3] = 0.0;
  dst[1][0] = srt.scale.y * (sinX * cosZ * sinY - cosX * sinZ);
  dst[1][1] = srt.scale.y * (sinX * sinZ * sinY + cosX * cosZ);
  dst[1][2] = srt.scale.y * (sinX * cosY);
  dst[1][3] = 0.0;
  dst[2][0] = srt.scale.z * (cosX * cosZ * sinY + sinX * sinZ);
  dst[2][1] = srt.scale.z * (cosX * sinZ * sinY - sinX * cosZ);
  dst[2][2] = srt.scale.z * (cosY * cosX);
  dst[2][3] = 0.0;
  dst[3][0] = srt.translation.x;
  dst[3][1] = srt.translation.y;
  dst[3][2] = srt.translation.z;
  dst[3][3] = 1.0;

  return dst;
}
glm::mat4 computeBoneMdl(u32 id, kpi::FolderData* bones) {
  glm::mat4 mdl(1.0f);

  auto& bone = bones->at<lib3d::Bone>(id);
  const auto parent = bone.getBoneParent();
  if (parent >= 0 && parent != id)
    mdl = computeBoneMdl(parent, bones);

  return mdl * computeMdlMtx(bone.getSRT());
}
std::vector<glm::mat4> Shape::getPosMtx(u64 mpid) const {
  std::vector<glm::mat4> out;

  const auto& mp = mMatrixPrimitives[mpid];

  auto& mdl = *getModel(this);
  ModelAccessor mdl_ac{const_cast<kpi::IDocumentNode*>(
      dynamic_cast<const kpi::IDocumentNode*>(&mdl))};
  // if (!(getVcd().mBitfield &
  //       (1 << (int)libcube::gx::VertexAttribute::PositionNormalMatrixIndex))) {
  //   return {
  //       mdl_ac.getJoint(0 /* DEBUG */).get().calcSrtMtx(&mdl_ac.getJoints())};
  // }
  for (const auto it : mp.mDrawMatrixIndices) {
    const auto& drw = mdl.mDrawMatrices[it];
    glm::mat4x4 curMtx(1.0f);

    lib3d::SRT3 srt;
    srt.scale = {1, 1, 1};
    srt.rotation = {90, 0, 0};
    srt.translation = {0, 0, 0};
    auto mtx = computeMdlMtx(srt);
    // Rigid -- bone space
    if (drw.mWeights.size() == 1) {
      u32 boneID = drw.mWeights[0].boneId;
      curMtx = mdl_ac.getJoint(boneID).get().calcSrtMtx(&mdl_ac.getJoints());
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
