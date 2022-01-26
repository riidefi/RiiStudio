#pragma once

#include <core/common.h>
#include <core/kpi/Node2.hpp>
#include <librii/gfx/PixelOcclusion.hpp>
#include <string>
#include <vector>

namespace riistudio::lib3d {

struct Texture : public virtual kpi::IObject {

  virtual std::string getName() const { return "Untitled Texture"; }
  virtual void setName(const std::string& name) = 0;
  virtual s64 getId() const { return -1; }

  virtual u32 getDecodedSize(bool mip) const {
    u32 mipMapCount = getMipmapCount();
    if (mip && mipMapCount > 0) {
      u32 w = getWidth();
      u32 h = getHeight();
      u32 total = w * h * 4;
      for (u32 i = 0; i < mipMapCount; ++i) {
        w >>= 1;
        h >>= 1;
        total += w * h * 4;
      }
      return total;
    } else {
      return getWidth() * getHeight() * 4;
    }
  }
  virtual u32 getEncodedSize(bool mip) const = 0;
  virtual void decode(std::vector<u8>& out, bool mip) const = 0;

  virtual u32 getImageCount() const = 0;
  virtual void setImageCount(u32 c) = 0;

  u32 getMipmapCount() const {
    u32 image_count = getImageCount();
    assert(image_count > 0);
    return image_count - 1;
  }
  void setMipmapCount(u32 c) { setImageCount(c + 1); }

  virtual u16 getWidth() const = 0;
  virtual void setWidth(u16 width) = 0;
  virtual u16 getHeight() const = 0;
  virtual void setHeight(u16 height) = 0;

  using Occlusion = librii::gfx::PixelOcclusion;

  //! @brief Set the image encoder based on the expression profile. Pixels are
  //!		 not recomputed immediately.
  //!
  //! @param[in] optimizeForSize	If the texture should prefer filesize
  //! over quality when deciding an encoder.
  //! @param[in] color			If the texture is not grayscale.
  //! @param[in] occlusion		The pixel occlusion selection.
  //!
  virtual void setEncoder(bool optimizeForSize, bool color,
                          Occlusion occlusion) = 0;

  //! @brief Encode the texture based on the current encoder, width, height,
  //!		 etc. and supplied raw data.
  //!
  //! @param[in] rawRGBA
  //!				- Raw pointer to a RGBA32 pixel array.
  //!				- Must be sized width * height * 4.
  //!				- If mipmaps are configured, this must also
  //!				  include all additional mip levels.
  //!
  virtual void encode(const u8* rawRGBA) = 0;

  struct IObserver {
    virtual ~IObserver() = default;
    virtual void detach(const Texture* tex) {}
    virtual void update(const Texture* tex) {}
  };

  void notifyObservers() {
    // We don't call nextGenerationId here as this is called when reverting from
    // an undo
    for (auto* it : observers) {
      it->update(this);
    }
  }
  void onUpdate() {
    nextGenerationId();
    // (for shader recompilation)
    notifyObservers();
  }
  mutable std::vector<IObserver*> observers;

  virtual ~Texture() {
    for (auto* it : observers)
      it->detach(this);
  }

  // Updating this value will force a cache invalidation. However, perhaps not
  // all changes warrant a cache invalidation. If you had the base type, you
  // could hold onto a copy of the old texture to determine if said update is
  // necessary; but in that case, you wouldn't need this system anyway.
  //
  // The other issue is that it's up to the user to update the generation ID.
  // However, that is also not resolved by using the observer system.
  //
  virtual s32 getGenerationId() const { return mGenerationId; }
  virtual void nextGenerationId() { ++mGenerationId; }

  s32 mGenerationId = 0;
};

} // namespace riistudio::lib3d
