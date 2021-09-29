#pragma once

#include "core/common.h"

namespace librii::gfx {

struct MegaState {
  u32 cullMode = -1;
  u32 depthWrite;
  u32 depthCompare;
  u32 frontFace;

  u32 blendMode;
  u32 blendSrcFactor;
  u32 blendDstFactor;

  float poly_offset_factor = 0.0f;
  float poly_offset_units = 0.0f;
};

} // namespace librii::gfx
