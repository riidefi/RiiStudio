#include "EnumConverter.hpp"
#include <core/3d/gl.hpp>

namespace librii::gl {

using namespace gx;
#ifdef RII_GL
Result<int> translateCullMode(gx::CullMode cullMode) {
  switch (cullMode) {
  case gx::CullMode::All:
    return GL_FRONT_AND_BACK;
  case gx::CullMode::Front:
    return GL_FRONT;
  case gx::CullMode::Back:
    return GL_BACK;
  case gx::CullMode::None:
    return -1;
  }
  return std::unexpected(
      std::format("Invalid cull mode: {}", static_cast<u32>(cullMode)));
}

Result<int> translateBlendFactorCommon(gx::BlendModeFactor factor) {
  switch (factor) {
  case gx::BlendModeFactor::zero:
    return GL_ZERO;
  case gx::BlendModeFactor::one:
    return GL_ONE;
  case gx::BlendModeFactor::src_a:
    return GL_SRC_ALPHA;
  case gx::BlendModeFactor::inv_src_a:
    return GL_ONE_MINUS_SRC_ALPHA;
  case gx::BlendModeFactor::dst_a:
    return GL_DST_ALPHA;
  case gx::BlendModeFactor::inv_dst_a:
    return GL_ONE_MINUS_DST_ALPHA;
  case gx::BlendModeFactor::src_c:
  case gx::BlendModeFactor::inv_src_c:
    break;
  }
  EXPECT(false, "Invalid blend mode");
  return ~0;
}

Result<int> translateBlendSrcFactor(gx::BlendModeFactor factor) {
  switch (factor) {
  case gx::BlendModeFactor::src_c:
    return GL_DST_COLOR;
  case gx::BlendModeFactor::inv_src_c:
    return GL_ONE_MINUS_DST_COLOR;
  default:
    return translateBlendFactorCommon(factor);
  }
}

Result<int> translateBlendDstFactor(gx::BlendModeFactor factor) {
  switch (factor) {
  case gx::BlendModeFactor::src_c:
    return GL_SRC_COLOR;
  case gx::BlendModeFactor::inv_src_c:
    return GL_ONE_MINUS_SRC_COLOR;
  default:
    return translateBlendFactorCommon(factor);
  }
}

int translateCompareType(gx::Comparison compareType) {
  switch (compareType) {
  case gx::Comparison::NEVER:
    return GL_NEVER;
  case gx::Comparison::LESS:
    return GL_LESS;
  case gx::Comparison::EQUAL:
    return GL_EQUAL;
  case gx::Comparison::LEQUAL:
    return GL_LEQUAL;
  case gx::Comparison::GREATER:
    return GL_GREATER;
  case gx::Comparison::NEQUAL:
    return GL_NOTEQUAL;
  case gx::Comparison::GEQUAL:
    return GL_GEQUAL;
  case gx::Comparison::ALWAYS:
    return GL_ALWAYS;
  default:
    return GL_ALWAYS;
  }
}

u32 gxFilterToGl(gx::TextureFilter filter) {
  switch (filter) {
  default:
  case gx::TextureFilter::Linear:
    return GL_LINEAR;
  case gx::TextureFilter::Near:
    return GL_NEAREST;
  case gx::TextureFilter::lin_mip_lin:
    return GL_LINEAR_MIPMAP_LINEAR;
  case gx::TextureFilter::lin_mip_near:
    return GL_LINEAR_MIPMAP_NEAREST;
  case gx::TextureFilter::near_mip_lin:
    return GL_NEAREST_MIPMAP_LINEAR;
  case gx::TextureFilter::near_mip_near:
    return GL_NEAREST_MIPMAP_NEAREST;
  }
}
u32 gxTileToGl(gx::TextureWrapMode wrap) {
  switch (wrap) {
  default:
  case gx::TextureWrapMode::Clamp:
    return GL_CLAMP_TO_EDGE;
  case gx::TextureWrapMode::Repeat:
    return GL_REPEAT;
  case gx::TextureWrapMode::Mirror:
    return GL_MIRRORED_REPEAT;
  }
}

Result<gfx::MegaState>
translateGfxMegaState(const gx::LowLevelGxMaterial& matdata) {
  gfx::MegaState megaState;
  megaState.cullMode = TRY(librii::gl::translateCullMode(matdata.cullMode));
  // TODO: If compare is false, is depth masked?
  megaState.depthWrite = matdata.zMode.update;
  // TODO: zmode "compare" part no reference
  megaState.depthCompare =
      matdata.zMode.compare
          ? librii::gl::translateCompareType(matdata.zMode.function)
          : GL_ALWAYS;
  // megaState.depthCompare = material.ropInfo.depthTest ?
  // reverseDepthForCompareMode(translateCompareType(material.ropInfo.depthFunc))
  // : GfxCompareMode.ALWAYS;
  megaState.frontFace = GL_CW;

  const auto blendMode = matdata.blendMode;
  if (blendMode.type == gx::BlendModeType::none) {
    megaState.blendMode = 0;
  } else if (blendMode.type == gx::BlendModeType::blend) {
    megaState.blendMode = GL_FUNC_ADD;
    megaState.blendSrcFactor = TRY(translateBlendSrcFactor(blendMode.source));
    megaState.blendDstFactor = TRY(translateBlendDstFactor(blendMode.dest));
  } else if (blendMode.type == gx::BlendModeType::subtract) {
    megaState.blendMode = GL_FUNC_REVERSE_SUBTRACT;
    megaState.blendSrcFactor = GL_ONE;
    megaState.blendDstFactor = GL_ONE;
  } else if (blendMode.type == gx::BlendModeType::logic) {
    return std::unexpected("LOGIC mode is unsupported.\n");
  }
  return megaState;
}

static u32 GetPolygonMode(librii::gfx::PolygonMode p) {
  switch (p) {
  case librii::gfx::PolygonMode::Point:
    return GL_POINT;
  case librii::gfx::PolygonMode::Line:
    return GL_LINE;
  default:
  case librii::gfx::PolygonMode::Fill:
    return GL_FILL;
  }
}

void setGlState(const librii::gfx::MegaState& state) {
  if (state.blendMode != 0) {
    glEnable(GL_BLEND);
    glBlendEquation(state.blendMode);
    glBlendFunc(state.blendSrcFactor, state.blendDstFactor);
  } else {
    glDisable(GL_BLEND);
  }

  if (state.cullMode == -1) {
    glDisable(GL_CULL_FACE);
  } else {
    glEnable(GL_CULL_FACE);
    glCullFace(state.cullMode);
  }
  glFrontFace(state.frontFace);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(state.depthWrite ? GL_TRUE : GL_FALSE);
  glDepthFunc(state.depthCompare);

  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(state.poly_offset_factor, state.poly_offset_units);

  // WebGL doesn't support glPolygonMode
#ifndef __EMSCRIPTEN__
  glPolygonMode(GL_FRONT_AND_BACK, GetPolygonMode(state.fill));
#endif
}
#endif

Result<gx::ColorSelChanApi> getRasColorChannelID(gx::ColorSelChanApi v) {
  switch (v) {
  case gx::ColorSelChanApi::color0:
  case gx::ColorSelChanApi::alpha0:
  case gx::ColorSelChanApi::color0a0:
    return gx::ColorSelChanApi::color0a0;
  case gx::ColorSelChanApi::color1:
  case gx::ColorSelChanApi::alpha1:
  case gx::ColorSelChanApi::color1a1:
    return gx::ColorSelChanApi::color1a1;
  case gx::ColorSelChanApi::ind_alpha:
    return gx::ColorSelChanApi::ind_alpha;
  case gx::ColorSelChanApi::normalized_ind_alpha:
    return gx::ColorSelChanApi::normalized_ind_alpha;
  case gx::ColorSelChanApi::zero:
  case gx::ColorSelChanApi::null:
    return gx::ColorSelChanApi::zero;
  }
  EXPECT(false, "Invalid color channel selection");
}

} // namespace librii::gl
