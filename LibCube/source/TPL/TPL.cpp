#include "TPL.hpp"

#include "TextureDecoding.hpp"
#include "ogc/texture.h"

namespace libcube {

struct DescriptorReader
{
	struct Context
	{
		u32 fileHeaderStart;
		DolphinTPL& tpl;
		u32 descIdx;
	};

	static constexpr char name[] = "Descriptor Array";

	static void onRead(oishii::BinaryReader& reader, Context& ctx)
	{
		// TODO: No duplicate checking
		const s32 ofsTex = reader.read<s32>();
		const s32 ofsPlt = reader.read<s32>();


		ctx.tpl.mDescriptors.emplace_back(ofsTex ? ctx.tpl.mTextures.size() : -1, ofsPlt ? ctx.tpl.mPalettes.size() : -1);

		if (ofsTex)
		{
			// TODO: Rewrite this

			DolphinTPL::Texture tex;
			tex.mDimensions.width = reader.read<u16>();
			tex.mDimensions.height = reader.read<u16>();
			tex.mFormat = reader.read<u32>();
			const int ofsData = reader.read<u32>();
			tex.mWrapU = reader.read<u32>();
			tex.mWrapV = reader.read<u32>();
			tex.mFilterMin = reader.read<u32>();
			tex.mFilterMag = reader.read<u32>();
			tex.mLodBias = reader.read<f32>();
			tex.mEdgeLod = reader.read<u8>();
			tex.mLodMin = reader.read<u8>();
			tex.mLodMax = reader.read<u8>();
			tex.unpacked = reader.read<u8>();


			tex.data.resize(GetTexBufferSize(tex.mDimensions.width, tex.mDimensions.height, tex.mFormat, tex.mLodMax, tex.mLodMax));
			memcpy_s(tex.data.data(), tex.data.size(), reader.getStreamStart() + ctx.fileHeaderStart + ofsData, tex.data.size());

			ctx.tpl.mTextures.push_back(tex);
		}

		// TODO: Palettes
	}
};

void DolphinTPL::read(oishii::BinaryReader& reader)
{
	const u32 start = reader.tell();
	mRevision = reader.read<u32>();

	const u32 nDesc = reader.read<u32>();

	for (u32 i = 0; i < nDesc; ++i)
	{
		reader.dispatch<
			DescriptorReader,
			oishii::Indirection<0, u32, oishii::Whence::Current>
		>(DescriptorReader::Context{ start, *this, i });
	}
}

} // namespace libcube
