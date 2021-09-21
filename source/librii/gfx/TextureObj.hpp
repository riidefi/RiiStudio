#pragma once

#include <core/common.h>

namespace librii::gfx {

struct TextureObj {
  u32 active_id; // glActiveTexture(GL_TEXTURE0 + active_id)
  u32 image_id;  // glBindTexture(GL_TEXTURE_2D, image_id)
  u32 glMinFilter;
  u32 glMagFilter;
  u32 glWrapU;
  u32 glWrapV;
};

void UseTexObj(const TextureObj& obj);

} // namespace librii::gfx
