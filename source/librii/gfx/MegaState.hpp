#pragma once

#include "core/common.h"

namespace librii::gfx {

enum PolygonMode { Point, Line, Fill };

struct MegaState {
  u32 cullMode = ~static_cast<u32>(0);
  u32 depthWrite;
  u32 depthCompare;
  u32 frontFace;

  u32 blendMode;
  u32 blendSrcFactor;
  u32 blendDstFactor;

  PolygonMode fill = PolygonMode::Fill;

  float poly_offset_factor = 0.0f;
  float poly_offset_units = 0.0f;
};

} // namespace librii::gfx
