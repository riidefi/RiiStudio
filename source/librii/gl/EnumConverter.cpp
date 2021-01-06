#include <core/3d/gl.hpp>
#include <librii/gx.h>

namespace librii::gl {

using namespace gx;

u32 translateCullMode(gx::CullMode cullMode) {
  switch (cullMode) {
  case gx::CullMode::All:
    return GL_FRONT_AND_BACK;
  case gx::CullMode::Front:
    return GL_FRONT;
  case gx::CullMode::Back:
    return GL_BACK;
  case gx::CullMode::None:
    return -1;
  default:
    // printf("Invalid cull mode!\n");
    return GL_BACK;
  }
}

u32 translateBlendFactorCommon(gx::BlendModeFactor factor) {
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
  default:
    assert(!"Invalid blend mode");
    return ~0;
  }
}

u32 translateBlendSrcFactor(gx::BlendModeFactor factor) {
  switch (factor) {
  case gx::BlendModeFactor::src_c:
    return GL_DST_COLOR;
  case gx::BlendModeFactor::inv_src_c:
    return GL_ONE_MINUS_DST_COLOR;
  default:
    return translateBlendFactorCommon(factor);
  }
}

u32 translateBlendDstFactor(gx::BlendModeFactor factor) {
  switch (factor) {
  case gx::BlendModeFactor::src_c:
    return GL_SRC_COLOR;
  case gx::BlendModeFactor::inv_src_c:
    return GL_ONE_MINUS_SRC_COLOR;
  default:
    return translateBlendFactorCommon(factor);
  }
}

u32 translateCompareType(gx::Comparison compareType) {
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
  case gx::TextureFilter::linear:
    return GL_LINEAR;
  case gx::TextureFilter::near:
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

} // namespace librii::gl
