#pragma once

#include <oishii/types.hxx>
#include <vector>
#include "Util/TextureDimensions.hpp"
#include <oishii/reader/binary_reader.hxx>

namespace libcube {

// Assumption: One texture header per texture data
class DolphinTPL
{
public:
	DolphinTPL() = default;
	~DolphinTPL() = default;

	static constexpr char name[] = "Dolphin TPL";

	u32 mRevision;

	struct Texture
	{
		TextureDimensions<u16> mDimensions;
		u32 mFormat; // TODO
		u32 mWrapU, mWrapV; // TODO
		u32 mFilterMin, mFilterMag; // TODO
		f32 mLodBias;
		bool mEdgeLod;
		u8 mLodMin, mLodMax;
		u8 unpacked;
		std::vector<u8> data;
	};

	struct Palette
	{
		u16 nEntries;
		u8 unpacked; // ?
		u32 format;
		std::vector<u8> data;
	};

	std::vector<Texture> mTextures;
	std::vector<Palette> mPalettes;

	// Index of texture/palette vectors
	std::vector<std::pair<int, int>> mDescriptors;

public:
	static void onRead(oishii::BinaryReader& reader, DolphinTPL& ctx)
	{
		ctx.read(reader);
	}

private:
	void read(oishii::BinaryReader& reader);
};


}
