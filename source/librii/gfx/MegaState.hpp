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
};

} // namespace librii::gfx
