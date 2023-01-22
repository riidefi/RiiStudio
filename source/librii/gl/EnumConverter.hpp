#pragma once

#include <librii/gfx/MegaState.hpp>
#include <librii/gx.h>

namespace librii::gl {

// Converts to GL
Result<int> translateCullMode(gx::CullMode cullMode);
Result<int> translateBlendSrcFactor(gx::BlendModeFactor factor);
Result<int> translateBlendDstFactor(gx::BlendModeFactor factor);
int translateCompareType(gx::Comparison compareType);

u32 gxFilterToGl(gx::TextureFilter filter);
u32 gxTileToGl(gx::TextureWrapMode wrap);

[[nodiscard]] Result<gfx::MegaState>
translateGfxMegaState(const gx::LowLevelGxMaterial& matdata);

void setGlState(const librii::gfx::MegaState& state);

} // namespace librii::gl
