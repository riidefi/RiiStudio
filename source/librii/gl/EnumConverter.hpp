#pragma once

#include <librii/gfx/MegaState.hpp>
#include <librii/gx.h>

namespace librii::gl {

// Converts to GL
Result<u32> translateCullMode(gx::CullMode cullMode);
Result<u32> translateBlendSrcFactor(gx::BlendModeFactor factor);
Result<u32> translateBlendDstFactor(gx::BlendModeFactor factor);
u32 translateCompareType(gx::Comparison compareType);

u32 gxFilterToGl(gx::TextureFilter filter);
u32 gxTileToGl(gx::TextureWrapMode wrap);

[[nodiscard]] Result<gfx::MegaState>
translateGfxMegaState(const gx::LowLevelGxMaterial& matdata);

void setGlState(const librii::gfx::MegaState& state);

} // namespace librii::gl
