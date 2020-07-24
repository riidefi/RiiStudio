#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "GPUMaterial.hpp"

namespace libcube::gpu {

IND_MTX::operator glm::mat4() {
  const f32 s = powf(
      2.0f,
      static_cast<s8>(
          static_cast<u32>(col0.s0 | (col1.s1 << 2) | (col2.s2 << 4)) - 0x11));

  auto elem = [&](f32 in) -> f32 {
    return s * static_cast<f32>(in) / static_cast<f32>(1 << 10);
  };

  glm::mat4 m;
  m[0][0] = elem(col0.ma);
  m[1][0] = elem(col1.mc);
  m[2][0] = elem(col2.me);
  m[3][0] = 0.0f;

  m[0][1] = elem(col0.mb);
  m[1][1] = elem(col1.md);
  m[2][1] = elem(col2.mf);
  m[3][1] = 0.0f;

  m[0][2] = 0.0f;
  m[1][2] = 0.0f;
  m[2][2] = 1.0f;
  m[3][2] = 0.0f;

  m[0][3] = 0.0f;
  m[1][3] = 0.0f;
  m[2][3] = 0.0f;
  m[3][3] = 1.0f;
  return m;
}

IND_MTX::operator gx::IndirectMatrix() {
  gx::IndirectMatrix tmp;

  const glm::mat4 transformation = *this;

  glm::vec3 scale;
  glm::quat rotation;
  glm::vec3 translation;
  glm::vec3 skew;
  glm::vec4 perspective;
  glm::decompose(transformation, scale, rotation, translation, skew,
                 perspective);
  // rotation = glm::conjugate(rotation);

  tmp.scale[0] = scale.x;
  tmp.scale[1] = scale.y;

  tmp.rotate = glm::degrees(glm::eulerAngles(rotation).z);

  tmp.trans[0] = translation.x;
  tmp.trans[1] = translation.y;

  return tmp;
}

XF_TEXTURE::operator gx::TexCoordGen() {
  gx::TexCoordGen tmp;

  tmp.matrix = static_cast<libcube::gx::TexMatrix>(30 + id * 3);
  // tmp.DestinationCoordinateID = id;

  tmp.normalize = dualTex.normalize;
  tmp.postMatrix = static_cast<gx::PostTexMatrix>(
      dualTex.index + static_cast<u32>(gx::PostTexMatrix::Matrix0));

  switch (tex.texgentype) {
  case XF_TEXGEN_REGULAR:
    tmp.func = tex.projection == XF_TEX_STQ ? gx::TexGenType::Matrix3x4
                                            : gx::TexGenType::Matrix2x4;

    switch (tex.sourcerow) {
    case XF_GEOM_INROW:
      tmp.sourceParam = gx::TexGenSrc::Position;
      break;
    case XF_NORMAL_INROW:
      tmp.sourceParam = gx::TexGenSrc::Normal;
      break;
    case XF_BINORMAL_T_INROW:
      tmp.sourceParam = gx::TexGenSrc::Binormal;
      break;
    case XF_BINORMAL_B_INROW:
      tmp.sourceParam = gx::TexGenSrc::Tangent;
      break;
    case XF_TEX0_INROW:
    case XF_TEX1_INROW:
    case XF_TEX2_INROW:
    case XF_TEX3_INROW:
    case XF_TEX4_INROW:
    case XF_TEX5_INROW:
    case XF_TEX6_INROW:
    case XF_TEX7_INROW:
      tmp.sourceParam = static_cast<gx::TexGenSrc>(
          tex.sourcerow - XF_TEX0_INROW + (u32)gx::TexGenSrc::UV0);
      break;
    default:
      //assert(!"Invalid texgen");
      break;
    }

    return tmp;
  case XF_TEXGEN_EMBOSS_MAP:
  default:
    if (tex.sourcerow == XF_COLORS_INROW) {
      tmp.func = gx::TexGenType::SRTG;
      tmp.sourceParam = tex.texgentype == XF_TEXGEN_COLOR_STRGBC0
                            ? gx::TexGenSrc::Color0
                            : gx::TexGenSrc::Color1;
      return tmp;
    }

    tmp.func = static_cast<gx::TexGenType>(tex.embosslightshift +
                                           (u32)gx::TexGenType::Bump0);
    tmp.sourceParam = static_cast<gx::TexGenSrc>(tex.embosssourceshift +
                                                 (u32)gx::TexGenSrc::UV0);
    return tmp;
  }
}
LitChannel::operator gx::ChannelControl() {
  auto diffuse_fn = gx::DiffuseFunction::None;
  auto attn_fn = gx::AttenuationFunction::None;

  if (!attnSelect.Value()) {
    attn_fn = gx::AttenuationFunction::Specular;
  } else if (!attnEnable.Value()) {
    diffuse_fn = static_cast<gx::DiffuseFunction>(diffuseAtten.Value());
  } else {
    attn_fn = gx::AttenuationFunction::Spotlight;
    diffuse_fn = static_cast<gx::DiffuseFunction>(diffuseAtten.Value());
  }

  return {.enabled = lightFunc.Value() != 0,
          .Ambient = static_cast<gx::ColorSource>(ambsource.Value()),
          .Material = static_cast<gx::ColorSource>(matsource.Value()),
          .lightMask = static_cast<gx::LightID>(GetFullLightMask()),
          .diffuseFn = diffuse_fn,
          .attenuationFn = attn_fn};
}
void LitChannel::from(const gx::ChannelControl& ctrl) {
  lightFunc = ctrl.enabled;
  ambsource = static_cast<u32>(ctrl.Ambient);
  matsource = static_cast<u32>(ctrl.Material);
  SetFullLightMask(static_cast<u32>(ctrl.lightMask));
  diffuseAtten =
      static_cast<u32>((ctrl.attenuationFn == gx::AttenuationFunction::Specular)
                           ? gx::DiffuseFunction::None
                           : ctrl.diffuseFn);
  attnEnable = ctrl.attenuationFn != gx::AttenuationFunction::None;
  attnSelect = ctrl.attenuationFn != libcube::gx::AttenuationFunction::Specular;
}
} // namespace libcube::gpu
