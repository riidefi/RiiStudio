#include "model.hpp"
#include "polygon.hpp"

namespace riistudio::g3d {

std::span<const glm::vec2> Polygon::getUv(const libcube::Model& mdl,
                                          u64 chan) const {
  const auto* buf = reinterpret_cast<const Model&>(mdl).getBuf_Uv().findByName(
      mTexCoordBuffer[chan]);
  if (!buf) {
    return {};
  }
  return buf->mEntries;
}
std::span<const librii::gx::Color> Polygon::getClr(const libcube::Model& mdl,
                                                   u64 chan) const {
  const auto* buf = reinterpret_cast<const Model&>(mdl).getBuf_Clr().findByName(
      mColorBuffer[chan]);
  if (!buf) {
    return {};
  }
  return buf->mEntries;
}
std::span<const glm::vec3> Polygon::getPos(const libcube::Model& mdl) const {
  const auto* buf = reinterpret_cast<const Model&>(mdl).getBuf_Pos().findByName(
      mPositionBuffer);
  if (!buf) {
    return {};
  }
  return buf->mEntries;
}
std::span<const glm::vec3> Polygon::getNrm(const libcube::Model& mdl) const {
  const auto* buf = reinterpret_cast<const Model&>(mdl).getBuf_Nrm().findByName(
      mNormalBuffer);
  if (!buf) {
    return {};
  }
  return buf->mEntries;
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

u64 Polygon::addPos(libcube::Model& mdl, const glm::vec3& v) {
  auto* buf =
      reinterpret_cast<Model&>(mdl).getBuf_Pos().findByName(mPositionBuffer);
  assert(buf);
  return add_to_buffer(v, *buf);
}
u64 Polygon::addNrm(libcube::Model& mdl, const glm::vec3& v) {
  auto* buf =
      reinterpret_cast<Model&>(mdl).getBuf_Nrm().findByName(mNormalBuffer);
  assert(buf);
  return add_to_buffer(v, *buf);
}
u64 Polygon::addClr(libcube::Model& mdl, u64 chan, const glm::vec4& v) {
  auto* buf =
      reinterpret_cast<Model&>(mdl).getBuf_Clr().findByName(mColorBuffer[chan]);
  assert(buf);

  librii::gx::ColorF32 fclr;
  fclr.r = v[0];
  fclr.g = v[1];
  fclr.b = v[2];
  fclr.a = v[3];
  librii::gx::Color c = fclr;
  return add_to_buffer(c, *buf);
}
u64 Polygon::addUv(libcube::Model& mdl, u64 chan, const glm::vec2& v) {
  auto* buf = reinterpret_cast<Model&>(mdl).getBuf_Uv().findByName(
      mTexCoordBuffer[chan]);
  assert(buf);
  return add_to_buffer(v, *buf);
}

glm::mat4 computeBoneMdl(u32 id, kpi::ConstCollectionRange<lib3d::Bone> bones) {
  glm::mat4 mdl(1.0f);

  auto& bone = bones[id];
  const auto parent = bone.getBoneParent();
  if (parent >= 0 && parent != id)
    mdl = computeBoneMdl(parent, bones);

  return mdl * librii::math::calcXform(bone.getSRT());
}

using namespace librii;

void Polygon::initBufsFromVcd(lib3d::Model& _mdl) {
  auto& mdl = reinterpret_cast<Model&>(_mdl);
  // Will break skinned
  mCurrentMatrix = 0;

  for (auto [attr, type] : mVertexDescriptor.mAttributes) {
    if (type == gx::VertexAttributeType::None)
      continue;
    switch (attr) {
    case librii::gx::VertexAttribute::Position: {
      const auto id = mdl.getBuf_Pos().size();
      auto& buf = mdl.getBuf_Pos().add();
      buf.mId = id;
      buf.mName = "Pos" + std::to_string(id);
      mPositionBuffer = buf.mName;
      break;
    }
    case librii::gx::VertexAttribute::Normal: {
      const auto id = mdl.getBuf_Nrm().size();
      auto& buf = mdl.getBuf_Nrm().add();
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
        const auto id = mdl.getBuf_Uv().size();
        auto& buf = mdl.getBuf_Uv().add();
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
        const auto id = mdl.getBuf_Clr().size();
        auto& buf = mdl.getBuf_Clr().add();
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
