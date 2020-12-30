#include "model.hpp"
#include "polygon.hpp"

namespace riistudio::g3d {

const g3d::Model* Polygon::getParent() const {
  return dynamic_cast<const g3d::Model*>(childOf);
}
g3d::Model* Polygon::getMutParent() {
  return dynamic_cast<g3d::Model*>(childOf);
}

glm::vec2 Polygon::getUv(u64 chan, u64 id) const {
  assert(getParent());
  const auto* buf = getParent()->getBuf_Uv().findByName(mTexCoordBuffer[chan]);
  assert(buf);
  if (id >= buf->mEntries.size())
    return {};
  assert(id < buf->mEntries.size());
  return buf->mEntries[id];
}
glm::vec4 Polygon::getClr(u64 chan, u64 id) const {
  assert(getParent());
  const auto* buf = getParent()->getBuf_Clr().findByName(mColorBuffer[chan]);
  assert(buf);
  if (id >= buf->mEntries.size())
    return {};
  assert(id < buf->mEntries.size());
  return static_cast<librii::gx::ColorF32>(buf->mEntries[id]);
}
glm::vec3 Polygon::getPos(u64 id) const {
  assert(getParent());
  const auto* buf = getParent()->getBuf_Pos().findByName(mPositionBuffer);
  assert(buf);
  if (id >= buf->mEntries.size())
    return {};
  assert(id < buf->mEntries.size());
  return buf->mEntries[id];
}
glm::vec3 Polygon::getNrm(u64 id) const {
  assert(getParent());
  const auto* buf = getParent()->getBuf_Nrm().findByName(mNormalBuffer);
  assert(buf);
  assert(id < buf->mEntries.size());
  return buf->mEntries[id];
}

template <typename X, typename Y>
auto add_to_buffer(const X& entry, Y& buf) -> u16 {
  const auto found = std::find(buf.mEntries.begin(), buf.mEntries.end(), entry);
  if (found != buf.mEntries.end()) {
    return found - buf.mEntries.begin();
  }

  buf.mEntries.push_back(entry);
  return buf.mEntries.size() - 1;
};

u64 Polygon::addPos(const glm::vec3& v) {
  assert(getMutParent());
  auto* buf = getMutParent()->getBuf_Pos().findByName(mPositionBuffer);
  assert(buf);
  return add_to_buffer(v, *buf);
}
u64 Polygon::addNrm(const glm::vec3& v) {
  assert(getMutParent());
  auto* buf = getMutParent()->getBuf_Nrm().findByName(mNormalBuffer);
  assert(buf);
  return add_to_buffer(v, *buf);
}
u64 Polygon::addClr(u64 chan, const glm::vec4& v) {
  assert(getMutParent());
  auto* buf = getMutParent()->getBuf_Clr().findByName(mColorBuffer[chan]);
  assert(buf);

  librii::gx::ColorF32 fclr;
  fclr.r = v[0];
  fclr.g = v[1];
  fclr.b = v[2];
  fclr.a = v[3];
  librii::gx::Color c = fclr;
  return add_to_buffer(c, *buf);
}
u64 Polygon::addUv(u64 chan, const glm::vec2& v) {
  assert(getMutParent());
  auto* buf = getMutParent()->getBuf_Uv().findByName(mTexCoordBuffer[chan]);
  assert(buf);
  return add_to_buffer(v, *buf);
}

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
glm::mat4 computeBoneMdl(u32 id, kpi::ConstCollectionRange<lib3d::Bone> bones) {
  glm::mat4 mdl(1.0f);

  auto& bone = bones[id];
  const auto parent = bone.getBoneParent();
  if (parent >= 0 && parent != id)
    mdl = computeBoneMdl(parent, bones);

  return mdl * computeMdlMtx(bone.getSRT());
}

const Model* getModel(const Polygon* shp) {
  assert(shp->getParent());
  return dynamic_cast<const Model*>(shp->getParent());
}
std::vector<glm::mat4> Polygon::getPosMtx(u64 mpid) const {
  std::vector<glm::mat4> out;

  const auto& mp = mMatrixPrimitives[mpid];

  const g3d::Model& mdl_ac = *getModel(this);

  const auto handle_drw = [&](const libcube::DrawMatrix& drw) {
    glm::mat4x4 curMtx(1.0f);

    // Rigid -- bone space
    if (drw.mWeights.size() == 1) {
      u32 boneID = drw.mWeights[0].boneId;
      curMtx = const_cast<Bone&>(mdl_ac.getBones()[boneID])
                   .calcSrtMtx(mdl_ac.getBones());
    } else {
      // already world space
    }
    out.push_back(curMtx);
  };

  if (mp.mDrawMatrixIndices.empty()) {
    if (mdl_ac.mDrawMatrices.size() > mCurrentMatrix) {
      handle_drw(mdl_ac.mDrawMatrices[mCurrentMatrix]);
    } else {
      // todo: is there really no rigging info here..?
      libcube::DrawMatrix tmp;
      tmp.mWeights.emplace_back(0, 1.0f);
      handle_drw(tmp);
    }
  } else {
    for (const auto it : mp.mDrawMatrixIndices) {
      handle_drw(mdl_ac.mDrawMatrices[it]);
    }
  }
  return out;
}

using namespace librii;

void Polygon::initBufsFromVcd() {
  // Will break skinned
  mCurrentMatrix = 0;

  for (auto [attr, type] : mVertexDescriptor.mAttributes) {
    if (type == gx::VertexAttributeType::None)
      continue;
    switch (attr) {
    case librii::gx::VertexAttribute::Position: {
      const auto id = getMutParent()->getBuf_Pos().size();
      auto& buf = getMutParent()->getBuf_Pos().add();
      buf.mId = id;
      buf.mName = "Pos" + std::to_string(id);
      mPositionBuffer = buf.mName;
      break;
    }
    case librii::gx::VertexAttribute::Normal: {
      const auto id = getMutParent()->getBuf_Nrm().size();
      auto& buf = getMutParent()->getBuf_Nrm().add();
      buf.mName = "Nrm" + std::to_string(id);
      buf.mId = id;
      mNormalBuffer = buf.mName;
      break;
    }
    case gx::VertexAttribute::TexCoord0:
    case gx::VertexAttribute::TexCoord1:
    case gx::VertexAttribute::TexCoord2:
    case gx::VertexAttribute::TexCoord3:
    case gx::VertexAttribute::TexCoord4:
    case gx::VertexAttribute::TexCoord5:
    case gx::VertexAttribute::TexCoord6:
    case gx::VertexAttribute::TexCoord7: {
      const auto chan = static_cast<int>(attr) -
                        static_cast<int>(gx::VertexAttribute::TexCoord0);

      if (mTexCoordBuffer[0].empty()) {
        const auto id = getMutParent()->getBuf_Uv().size();
        auto& buf = getMutParent()->getBuf_Uv().add();
        buf.mName = "Uv" + std::to_string(id);
        buf.mId = id;
        mTexCoordBuffer[chan] = buf.mName;
      } else {
        mTexCoordBuffer[chan] = mTexCoordBuffer[0];
      }
      break;
    }
    case gx::VertexAttribute::Color0:
    case gx::VertexAttribute::Color1: {
      const auto chan = static_cast<int>(attr) -
                        static_cast<int>(gx::VertexAttribute::Color0);

      if (mColorBuffer[0].empty()) {
        const auto id = getMutParent()->getBuf_Clr().size();
        auto& buf = getMutParent()->getBuf_Clr().add();
        buf.mName = "Clr" + std::to_string(id);
        buf.mId = id;
        mColorBuffer[chan] = buf.mName;
      } else {
        mColorBuffer[chan] = mColorBuffer[0];
      }
      break;
    }
    default:
      break;
    }
  }
}

} // namespace riistudio::g3d
