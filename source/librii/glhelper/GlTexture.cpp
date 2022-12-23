#include "GlTexture.hpp"
#include <core/3d/gl.hpp>

IMPORT_STD;

namespace librii::glhelper {

GlTexture::~GlTexture() {
#ifdef RII_GL
  if (mGlId != ~0)
    glDeleteTextures(1, &mGlId);
#endif
}

std::optional<GlTexture> GlTexture::makeTexture(const riistudio::lib3d::Texture& tex) {
#ifdef RII_GL
  static std::vector<u8> data(1024 * 1024 * 4 * 2);

  u32 gl_id;
  glGenTextures(1, &gl_id);
  glBindTexture(GL_TEXTURE_2D, gl_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex.getMipmapCount());
  tex.decode(data, true);

  u32 slide = 0;
  for (u32 i = 0; i <= tex.getMipmapCount(); ++i) {
    glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, tex.getWidth() >> i,
                 tex.getHeight() >> i, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 data.data() + slide);
    slide += (tex.getWidth() >> i) * (tex.getHeight() >> i) * 4;
  }

  return GlTexture{gl_id};
#else
  return std::nullopt;
#endif
}

} // namespace librii::glhelper
