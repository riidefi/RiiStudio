#pragma once

#include <frontend/widgets/Image.hpp>

namespace riistudio::frontend {

// Rather than re-uploading texture data every frame, this structure holds onto
// texture data from the last frame.
//
// TODO: Really one texture only needs to be uploaded once. We could have some
// global database of UUID to GL tex to increase sharing.
struct Lib3dCachedImagePreview {
  riistudio::frontend::ImagePreview mImg;
  riistudio::lib3d::GenerationIDTracked::GenerationID last_tex_uuid = -1;

  bool isHolding(const riistudio::lib3d::Texture& tex) {
    return last_tex_uuid == tex.getGenerationId();
  }

  void draw(const riistudio::lib3d::Texture& tex, float width = -1.0f,
            float height = -1.0f, bool mip_slider = true) {
    if (!isHolding(tex)) {
      mImg.setFromImage(tex);
      last_tex_uuid = tex.getGenerationId();
    }
    mImg.draw(width, height, mip_slider);
  }
};

} // namespace riistudio::frontend
