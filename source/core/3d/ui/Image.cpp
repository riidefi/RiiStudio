#include "Image.hpp"
#include <algorithm>
#include <core/3d/gl.hpp>
#include <imgui/imgui.h>
#undef min

namespace riistudio::frontend {

ImagePreview::ImagePreview() {}
ImagePreview::~ImagePreview() {
  if (mTexUploaded) {
    glDeleteTextures(1, &mGpuTexId);
    mTexUploaded = false;
  }
}

void ImagePreview::setFromImage(u32 _width, u32 _height, u32 _num_mip,
                                std::vector<u8>&& _buffer) {
  width = _width;
  height = _height;
  mNumMipMaps = _num_mip;
  mDecodeBuf = std::move(_buffer);

  mLod = std::min(static_cast<u32>(mLod), mNumMipMaps);

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

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mNumMipMaps);
  u32 slide = 0;
  for (u32 i = 0; i <= mNumMipMaps; ++i) {
    glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, width, height >> i, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, mDecodeBuf.data() + slide);
    slide += (width >> i) * (height >> i) * 4;
  }
  mDecodeBuf.clear();
}

void ImagePreview::draw(float wd, float ht, bool mip_slider) {
  if (!mTexUploaded) {
    ImGui::Text("No image to display");
    return;
  }

  glBindTexture(GL_TEXTURE_2D, mGpuTexId);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, static_cast<f32>(mLod));
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, static_cast<f32>(mLod));

  ImGui::Image(
      (void*)(intptr_t)mGpuTexId,
      ImVec2((wd > 0 ? wd : width) * mScale, (ht > 0 ? ht : height) * mScale));

  if (ImGui::BeginPopupContextWindow()) {
    ImGui::SliderFloat("Image Preview Scale", &mScale, 0.0f, 10.0f);
    ImGui::EndPopup();
  }

  if (mip_slider && mNumMipMaps > 0)
    ImGui::SliderInt("LOD", &mLod, 0, mNumMipMaps);
}
} // namespace riistudio::frontend
