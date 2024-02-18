

#include <utility>

#include "librii/j3d/data/TextureData.hpp"
#include "plugins/gc/Export/Texture.hpp"

namespace librii::jpa {

struct TextureBlock : public libcube::Texture {
  librii::j3d::Tex tex;
private:
  std::vector<u8> mInternalData;
  std::string mName;
public:
  TextureBlock(librii::j3d::Tex tex, std::vector<u8> data)
      : tex(tex), mInternalData(std::move(data)) {
  }

  librii::gx::TextureFormat getTextureFormat() const override {
    return tex.mFormat;
  }
  void setTextureFormat(librii::gx::TextureFormat format) override {}
  std::span<const u8> getData() const override { return mInternalData; }
  std::span<u8> getData() override { return mInternalData; }
  void resizeData() override {}
  const u8* getPaletteData() const override { return nullptr; }
  u32 getPaletteFormat() const override { return tex.mPaletteFormat; }
  void setLod(bool custom, f32 min_, f32 max_) override {}
  void setSourcePath(std::string_view) override {}
  f32 getMinLod() const override { return tex.mMinLod; }
  f32 getMaxLod() const override { return tex.mMaxLod; }
  std::string getSourcePath() const override { return ""; }
  std::string getName() const override { return mName; }
  void setName(const std::string& name) override { mName = name; }

  void setName(const char* name) {
    mName = name;
  }


  u32 getImageCount() const override { return tex.mMipmapLevel; };
  void setImageCount(u32 c) override{};

  u16 getWidth() const override { return tex.mWidth; };
  void setWidth(u16 width) override{};
  u16 getHeight() const override { return tex.mHeight; };
  void setHeight(u16 height) override{};

};

} // namespace librii::jpa
