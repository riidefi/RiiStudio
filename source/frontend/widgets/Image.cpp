#include "Image.hpp"
#include <algorithm>
#include <core/3d/gl.hpp>
#include <imgui/imgui.h>
#undef min

namespace riistudio::frontend {

ImagePreview::ImagePreview() {}
ImagePreview::~ImagePreview() {
#ifdef RII_GL
  if (mTexUploaded) {
    glDeleteTextures(1, &mGpuTexId);
    mTexUploaded = false;
  }
#endif
}

void ImagePreview::setFromImage(const lib3d::Texture& tex) {
#ifdef RII_GL
  width = tex.getWidth();
  height = tex.getHeight();
  mNumMipMaps = tex.getMipmapCount();
  mLod = std::min(static_cast<u32>(mLod), mNumMipMaps);
  tex.decode(mDecodeBuf, true);

  if (mTexUploaded) {
    glDeleteTextures(1, &mGpuTexId);
  }
  if (mDecodeBuf.size() && width && height) {
    glGenTextures(1, &mGpuTexId);
  } else {
    mTexUploaded = false;
    return;
  }
  mTexUploaded = true;

  glBindTexture(GL_TEXTURE_2D, mGpuTexId);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex.getMipmapCount());
  u32 slide = 0;
  for (u32 i = 0; i <= tex.getMipmapCount(); ++i) {
    glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, tex.getWidth() >> i,
                 tex.getHeight() >> i, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 mDecodeBuf.data() + slide);
    slide += (tex.getWidth() >> i) * (tex.getHeight() >> i) * 4;
  }
  mDecodeBuf.clear();
#endif
}

void ImagePreview::draw(float wd, float ht, bool mip_slider) {
  if (!mTexUploaded) {
    ImGui::Text("No image to display");
    return;
  }

#ifdef RII_GL
  glBindTexture(GL_TEXTURE_2D, mGpuTexId);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, static_cast<f32>(mLod));
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, static_cast<f32>(mLod));

  ImGui::Image(
      (void*)(intptr_t)mGpuTexId,
      ImVec2((wd > 0 ? wd : width) * mScale, (ht > 0 ? ht : height) * mScale));
#endif

  if (ImGui::BeginPopupContextWindow()) {
    ImGui::SliderFloat("Image Preview Scale", &mScale, 0.0f, 10.0f);
    ImGui::EndPopup();
  }

  if (mip_slider && mNumMipMaps > 0)
    ImGui::SliderInt("LOD", &mLod, 0, mNumMipMaps);
}
} // namespace riistudio::frontend
