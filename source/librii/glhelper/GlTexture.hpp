#pragma once

#include <core/3d/Texture.hpp>
#include <core/common.h>
#include <optional>

namespace librii::glhelper {

class GlTexture {
public:
  ~GlTexture();
  GlTexture(GlTexture&& old) : mGlId(old.mGlId) { old.mGlId = ~0; }
  GlTexture(const GlTexture&) = delete;
  GlTexture(const riistudio::lib3d::Texture& tex) {
    auto opt = makeTexture(tex);
    assert(opt.has_value());
    this->mGlId = opt->mGlId;
    opt->mGlId = ~0;
  }

  u32 getGlId() const { return mGlId; }

private:
  u32 mGlId;

  GlTexture(u32 gl_id) : mGlId(gl_id) {}

public:
  static std::optional<GlTexture>
  makeTexture(const riistudio::lib3d::Texture& tex);
};

} // namespace librii::glhelper
