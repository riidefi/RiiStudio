#pragma once

#include "GXMaterial.hpp"
#include <core/util/string_builder.hpp> // StringBuilder
#include <llvm/Support/Error.h>         // llvm::Error
#include <string>                       // std::string
#include <vendor/glm/vec4.hpp>

namespace libcube {

// Converts to GL
u32 translateCullMode(gx::CullMode cullMode);
u32 translateBlendFactorCommon(gx::BlendModeFactor factor);
u32 translateBlendSrcFactor(gx::BlendModeFactor factor);
u32 translateBlendDstFactor(gx::BlendModeFactor factor);
u32 translateCompareType(gx::Comparison compareType);

void translateGfxMegaState(MegaState& megaState, GXMaterial& material);
gx::ColorSelChanApi getRasColorChannelID(gx::ColorSelChanApi v);

} // namespace libcube
