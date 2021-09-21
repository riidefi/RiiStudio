#include "TextureObj.hpp"
#include <core/3d/gl.hpp>

namespace librii::gfx {

void UseTexObj(const TextureObj& obj) {
#ifdef RII_GL
  glActiveTexture(GL_TEXTURE0 + obj.active_id);
  glBindTexture(GL_TEXTURE_2D, obj.image_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, obj.glMinFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, obj.glMagFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, obj.glWrapU);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, obj.glWrapV);
#endif
}

} // namespace librii::gfx
