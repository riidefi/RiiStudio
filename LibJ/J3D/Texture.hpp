#pragma once

#include <LibCore/common.h>
#include <LibCube/GX/VertexTypes.hpp>
#include <vector>

#include <LibCube/Export/Texture.hpp>

#include <ThirdParty/fa5/IconsFontAwesome5.h>

namespace libcube::jsystem {

struct TextureData
{
	std::string mName; // For linking

	u8 mFormat = 6;
	bool bTransparent = false;
	u16 mWidth, mHeight;

	u8 mPaletteFormat;
	// Not resolved or supported currently
	u16 nPalette;
	u32 ofsPalette;

	s8 mMinLod;
	s8 mMaxLod;
	u8 mMipmapLevel = 0;

	std::vector<u8> mData;
};

struct Texture : public TextureData, public libcube::Texture
{
	PX_TYPE_INFO_EX("J3D Texture", "j3d_tex", "J::Texture", ICON_FA_IMAGES, ICON_FA_IMAGE);


	std::string getName() const override { return mName; }
	void setName(const std::string& name) override { mName = name; }

    u32 getTextureFormat() const override
	{
		return mFormat;
	}

	void setTextureFormat(u32 format) override
	{
		mFormat = format;
	}
    u32 getMipmapCount() const override
	{
		return mMipmapLevel;
	}
    const u8* getData() const override
	{
		return mData.data();
	}
	u8* getData() override
	{
		return mData.data();
	}
	void resizeData() override
	{
		mData.resize(getEncodedSize(false));
	}

    const u8* getPaletteData() const override
	{
		return nullptr;
	}
    u32 getPaletteFormat() const override
	{
		return 0;
	}

	u16 getWidth() const override
	{
		return mWidth;
	}
	void setWidth(u16 width) override { mWidth = width; }
	u16 getHeight() const override
	{
		return mHeight;
	}
	void setHeight(u16 height) override { mHeight = height; }
};

}
