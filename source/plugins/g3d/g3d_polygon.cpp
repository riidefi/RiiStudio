#include "polygon.hpp"

#include "model.hpp"

namespace riistudio::g3d {

glm::vec2 Polygon::getUv(u64 chan, u64 id) const {
  // Assumption: Parent of parent model is a collection with children.
  const kpi::IDocumentNode* parent = getParent();
  assert(parent);
  const auto* posBufs = parent->getFolder<TextureCoordinateBuffer>();
  assert(posBufs);
  for (const auto& p : *posBufs) {
    if (p->getName() == mTexCoordBuffer[chan]) {
      auto* x = dynamic_cast<TextureCoordinateBuffer*>(p.get());
      return x->mEntries[id];
    }
  }
  return {};
}
glm::vec4 Polygon::getClr(u64 id) const {
  const kpi::IDocumentNode* parent = getParent();
  assert(parent);
  const auto* posBufs = parent->getFolder<ColorBuffer>();
  assert(posBufs);
  for (const auto& p : *posBufs) {
    if (p->getName() == mColorBuffer[0]) {
      const auto* buf = dynamic_cast<ColorBuffer*>(p.get());
      const auto entry = buf->mEntries[id];
      const libcube::gx::ColorF32 ef32 = entry;
      return ef32;
    }
  }
  return {};
}
glm::vec3 Polygon::getPos(u64 id) const {
  const kpi::IDocumentNode* parent = getParent();
  assert(parent);
  const auto* posBufs = parent->getFolder<PositionBuffer>();
  assert(posBufs);
  for (const auto& p : *posBufs) {
    if (p->getName() == mPositionBuffer)
      return dynamic_cast<PositionBuffer*>(p.get())->mEntries[id];
  }
  return {};
}
glm::vec3 Polygon::getNrm(u64 id) const {
  const kpi::IDocumentNode* parent = getParent();
  assert(parent);
  const auto* posBufs = parent->getFolder<NormalBuffer>();
  assert(posBufs);
  for (const auto& p : *posBufs) {
    if (p->getName() == mNormalBuffer)
      return dynamic_cast<NormalBuffer*>(p.get())->mEntries[id];
  }
  return {};
}
u64 Polygon::addPos(const glm::vec3& v) { return 0; }
u64 Polygon::addNrm(const glm::vec3& v) { return 0; }
u64 Polygon::addClr(u64 chan, const glm::vec4& v) { return 0; }
u64 Polygon::addUv(u64 chan, const glm::vec2& v) { return 0; }

void Polygon::addTriangle(std::array<SimpleVertex, 3> tri) {}

glm::mat4 computeMdlMtx(const lib3d::SRT3& srt) {
  glm::mat4 dst(1.0f);

  //	dst = glm::translate(dst, srt.translation);
  //	dst = dst * glm::eulerAngleZYX(glm::radians(srt.rotation.x),
  // glm::radians(srt.rotation.y), glm::radians(srt.rotation.z)); 	return
  // glm::scale(dst, srt.scale);

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

const G3DModel* getModel(const Polygon* shp) {
  assert(shp->getParent());
  return dynamic_cast<const G3DModel*>(shp->getParent());
}
std::vector<glm::mat4> Polygon::getPosMtx(u64 mpid) const {
  std::vector<glm::mat4> out;

  const auto& mp = mMatrixPrimitives[mpid];

  auto& mdl = *getModel(this);
  G3DModelAccessor mdl_ac{const_cast<kpi::IDocumentNode*>(
      dynamic_cast<const kpi::IDocumentNode*>(&mdl))};

  const auto handle_drw = [&](const libcube::DrawMatrix& drw) {
    glm::mat4x4 curMtx(1.0f);

    lib3d::SRT3 srt;
    srt.scale = {1, 1, 1};
    srt.rotation = {90, 0, 0};
    srt.translation = {0, 0, 0};
    auto mtx = computeMdlMtx(srt);
    // Rigid -- bone space
    if (drw.mWeights.size() == 1) {
      u32 boneID = drw.mWeights[0].boneId;
      curMtx = mdl_ac.getBone(boneID).get().calcSrtMtx(&mdl_ac.getBones());
    } else {
      // already world space
    }
    out.push_back(curMtx);
  };

  if (mp.mDrawMatrixIndices.empty()) {
    if (mdl.mDrawMatrices.size() > mCurrentMatrix) {
      handle_drw(mdl.mDrawMatrices[mCurrentMatrix]);
    }
  } else {
    for (const auto it : mp.mDrawMatrixIndices) {
      handle_drw(mdl.mDrawMatrices[it]);
    }
  }
  return out;
}

} // namespace riistudio::g3d
