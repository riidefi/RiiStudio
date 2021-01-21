#include "GlTexture.hpp"
#include <array>
#include <core/3d/gl.hpp>
#include <vector>

namespace riistudio::lib3d {

GlTexture::~GlTexture() {
  if (mGlId != ~0)
    glDeleteTextures(1, &mGlId);
}

std::optional<GlTexture> GlTexture::makeTexture(const lib3d::Texture& tex) {
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
}

} // namespace riistudio::lib3d
