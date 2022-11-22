#pragma once

#include <algorithm>
#include <core/3d/i3dmodel.hpp>
#include <librii/gx/Texture.hpp>
#include <librii/image/ImagePlatform.hpp>
#include <string_view>

namespace libcube {

struct Texture : public riistudio::lib3d::Texture {
  // PX_TYPE_INFO("GameCube Texture", "gc_tex", "GC::Texture");

  inline u32 getEncodedSize(bool mip) const override {
    return librii::gx::computeImageSize(
        getWidth(), getHeight(), getTextureFormat(), mip ? getImageCount() : 0);
  }
  inline void decode(std::vector<u8>& out, bool mip) const override {
    u32 size = getDecodedSize(mip);
    if (size == 0)
      return;
    size = std::max(size, u32(1024 * 1024 * 4));
    // assert(size);

    if (out.size() < size) {
      out.resize(size);
    }

    if (librii::gx::IsPaletteFormat(getTextureFormat())) {
      // Palettes not supported
      return;
    }

    librii::image::transform(
        out.data(), getWidth(), getHeight(), getTextureFormat(),
        librii::gx::TextureFormat::Extension_RawRGBA32, getData(), getWidth(),
        getHeight(), mip ? getMipmapCount() : 0);
  }

  virtual librii::gx::TextureFormat getTextureFormat() const = 0;
  virtual void setTextureFormat(librii::gx::TextureFormat format) = 0;
  virtual const u8* getData() const = 0;
  virtual u8* getData() = 0;
  virtual void resizeData() = 0;
  virtual const u8* getPaletteData() const = 0;
  virtual u32 getPaletteFormat() const = 0;

  //! @brief Set the image encoder based on the expression profile. Pixels are
  //! not recomputed immediately.
  //!
  //! @param[in] optimizeForSize	If the texture should prefer filesize
  //! over quality when deciding an encoder.
  //! @param[in] color			If the texture is not grayscale.
  //! @param[in] occlusion		The pixel occlusion selection.
  //!
  void setEncoder(bool optimizeForSize, bool color,
                  Occlusion occlusion) override {
    // TODO
    setTextureFormat(librii::gx::TextureFormat::CMPR);
  }

  //! @brief Encode the texture based on the current encoder, width, height,
  //! etc. and supplied raw data.
  //!
  //! @param[in] rawRGBA
  //!				- Raw pointer to a RGBA32 pixel array.
  //!				- Must be sized width * height * 4.
  //!				- If mipmaps are configured, this must also
  //! include all additional mip levels.
  //!
  void encode(const u8* rawRGBA) override {
    resizeData();

    librii::image::transform(getData(), getWidth(), getHeight(),
                             gx::TextureFormat::Extension_RawRGBA32,
                             getTextureFormat(), rawRGBA, getWidth(),
                             getHeight(), getMipmapCount());
  }

  virtual void setLod(bool custom, f32 min_, f32 max_) = 0;
  virtual void setSourcePath(std::string_view) = 0;
  virtual f32 getMinLod() const = 0;
  virtual f32 getMaxLod() const = 0;
  virtual std::string getSourcePath() const = 0;
};

} // namespace libcube
