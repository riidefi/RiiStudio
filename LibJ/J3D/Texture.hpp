#pragma once

#include <LibCore/common.h>
#include <LibCube/GX/VertexTypes.hpp>
#include <vector>

namespace libcube::jsystem {

struct TextureData
{
	std::string mName; // For linking

	u8 mFormat;
	bool bTransparent;
	u16 mWidth, mHeight;

	u8 mPaletteFormat;
	// Not resolved or supported currently
	u16 nPalette;
	u32 ofsPalette;

	s8 mMinLod;
	s8 mMaxLod;
	u8 mMipmapLevel;

	std::vector<u8> mData;
};

struct Texture : public TextureData
{

};

}
