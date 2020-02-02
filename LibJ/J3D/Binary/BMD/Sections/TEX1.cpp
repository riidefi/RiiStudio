#include <LibJ/J3D/Binary/BMD/Sections.hpp>
#include <ThirdParty/ogc/texture.h>
#include <map>

namespace libcube::jsystem {

struct Tex
{
	u8 mFormat;
	bool bTransparent;
	u16 mWidth, mHeight;
	u8 mWrapU, mWrapV;
	u8 mPaletteFormat;
	u16 nPalette;
	u32 ofsPalette;
	bool bMipMap;
	bool bEdgeLod;
	bool bBiasClamp;
	u8 mMaxAniso;
	u8 mMinFilter;
	u8 mMagFilter;
	s8 mMinLod;
	s8 mMaxLod;
	u8 mMipmapLevel;
	s16 mLodBias;
	u32 ofsTex;

	void transfer(oishii::BinaryReader& stream)
	{
		stream.transfer(mFormat);
		stream.transfer(bTransparent);
		stream.transfer(mWidth);
		stream.transfer(mHeight);
		stream.transfer(mWrapU);
		stream.transfer(mWrapV);
		stream.seek(1);
		stream.transfer(mPaletteFormat);
		stream.transfer(nPalette);
		stream.transfer(ofsPalette);
		stream.transfer(bMipMap);
		stream.transfer(bEdgeLod);
		stream.transfer(bBiasClamp);
		stream.transfer(mMaxAniso);
		stream.transfer(mMinFilter);
		stream.transfer(mMagFilter);
		stream.transfer(mMinLod);
		stream.transfer(mMaxLod);
		stream.transfer(mMipmapLevel);
		stream.transfer(mLodBias);
		stream.transfer(ofsTex);
	}
};


void readTEX1(BMDOutputContext& ctx)
{
    auto& reader = ctx.reader;
	if (!enterSection(ctx, 'TEX1')) return;
	
	ScopedSection g(ctx.reader, "Textures");

    u16 size = reader.read<u16>();
    reader.read<u16>();

    const auto [ofsHeaders, ofsNameTable] = reader.readX<s32, 2>();
    reader.seekSet(g.start);

	std::vector<std::string> nameTable;
	{
		oishii::Jump j(reader, ofsNameTable);
		nameTable = readNameTable(reader);
	}
	reader.seek(ofsHeaders);
    for (int i = 0; i < size; ++i)
    {
		Tex tex;
		tex.transfer(reader);
		for (int i = 0; i < ctx.mdl.getMaterials().size(); ++i)
		{
			auto& mat = ctx.mdl.getMaterials()[i];
			for (auto& samp : mat.textures)
			{
				if (samp.btiId == i)
				{
					samp.mTexture = nameTable[i];
					samp.mWrapU = static_cast<gx::TextureWrapMode>(tex.mWrapU);
					samp.mWrapV = static_cast<gx::TextureWrapMode>(tex.mWrapV);
					samp.bMipMap = tex.bMipMap;
					samp.bEdgeLod = tex.bEdgeLod;
					samp.bBiasClamp = tex.bBiasClamp;
					samp.mMaxAniso = tex.mMaxAniso;
					samp.mMinFilter = tex.mMinFilter;
					samp.mMagFilter = tex.mMagFilter;
					samp.mLodBias = tex.mLodBias;
				}
			}
		}
		ctx.col.getTextures().push(std::make_unique<Texture>());
		auto& data = ctx.col.getTexture(ctx.col.getTextures().size());
		data.mName = nameTable[i];
		data.mFormat = tex.mFormat;
		data.mWidth = tex.mWidth;
		data.mHeight = tex.mHeight;
		data.mPaletteFormat = tex.mPaletteFormat;
		data.nPalette = tex.nPalette;
		data.ofsPalette = tex.ofsPalette;
		data.mMinLod = tex.mMinLod;
		data.mMaxLod = tex.mMaxLod;
		data.mMipmapLevel = tex.mMipmapLevel;

		const u32 texOfs = g.start + tex.ofsTex;
		const u32 texSize = GetTexBufferSize(tex.mWidth, tex.mHeight, tex.mFormat, tex.bMipMap, tex.mMaxLod);

		data.mData.resize(texSize);
		assert(texOfs + texSize < reader.endpos());
		memcpy_s(data.mData.data(), data.mData.size(), reader.getStreamStart() + texOfs, texSize);
    }
}

struct TEX1Node final : public oishii::v2::Node
{
	TEX1Node(const J3DModel& model, const J3DCollection& col)
		: mModel(model), mCol(col)
	{
		mId = "TEX1";
		mLinkingRestriction.alignment = 32;
	}

	struct TexNames : public oishii::v2::Node
	{
		TexNames(const J3DModel& mdl, const J3DCollection& col)
			: mMdl(mdl), mCol(col)
		{
			mId = "TexNames";
			getLinkingRestriction().setLeaf();
			getLinkingRestriction().alignment = 4;
		}

		Result write(oishii::v2::Writer& writer) const noexcept
		{
			std::vector<std::string> names;
			
			for (int i = 0; i < mCol.getTextures().size(); ++i)
				names.push_back(mCol.getTextures()[i].getName());
			writeNameTable(writer, names);

			return {};
		}

		const J3DModel& mMdl;
		const J3DCollection& mCol;
	};
	Result write(oishii::v2::Writer& writer) const noexcept override
	{
		writer.write<u32, oishii::EndianSelect::Big>('TEX1');
		writer.writeLink<s32>({ *this }, { *this, oishii::v2::Hook::EndOfChildren });

		writer.write<u16>((u16)mModel.getBones().size());
		writer.write<u16>(-1);

		writer.writeLink<s32>(
			oishii::v2::Hook(*this),
			oishii::v2::Hook("TexHeaders"));
		writer.writeLink<s32>(
			oishii::v2::Hook(*this),
			oishii::v2::Hook("TexNames"));
		
		return eResult::Success;
	}

	Result gatherChildren(NodeDelegate& d) const noexcept override
	{
		d.addNode(std::make_unique<TexNames>(mModel, mCol));

		return {};
	}

private:
	const J3DModel& mModel;
	const J3DCollection& mCol;
};

std::unique_ptr<oishii::v2::Node> makeTEX1Node(BMDExportContext& ctx)
{
	return std::make_unique<TEX1Node>(ctx.mdl, ctx.col);
}


}
