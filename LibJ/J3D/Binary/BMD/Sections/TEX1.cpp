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
		oishii::DebugExpectSized dbg(stream, 0x20);

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
		stream.seek(1);
		// stream.transfer(mLodBias);
		mLodBias = stream.read<s16>();
		// stream.transfer(ofsTex);
		ofsTex = stream.read<u32>();
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

	std::vector<std::pair<std::unique_ptr<Texture>, std::pair<u32, u32>>> texRaw;

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
		auto& inf = texRaw.emplace_back(std::make_unique<Texture>(), std::pair<u32, u32>{0, 0});
		auto& data = *inf.first.get();

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

		// ofs:size
		inf.second.first = g.start + ofsHeaders + i * 32 + tex.ofsTex;
		inf.second.second = GetTexBufferSize(tex.mWidth, tex.mHeight, tex.mFormat, tex.bMipMap, tex.mMaxLod);
	}

	// Deduplicate and read.
	// Assumption: Data will not be spliced

	std::vector<std::pair<u32, int>> uniques; // ofs : index
	for (int i = 0; i < size; ++i)
	{
		const auto found = std::find_if(uniques.begin(), uniques.end(), [&](const auto& it) { return it.first == texRaw[i].second.first; });
		if (found == uniques.end())
			uniques.emplace_back(texRaw[i].second.first, i);
	}

	int i = 0;
	for (const auto& it : uniques)
	{
		auto& texpair = texRaw[it.second];
		const auto [ofs, size] = texpair.second;
		std::unique_ptr<Texture> data = std::move(texpair.first);

		data->mData.resize(size);
		assert(ofs + size < reader.endpos());
		memcpy_s(data->mData.data(), data->mData.size(), reader.getStreamStart() + ofs, size);

		ctx.col.getTextures().push(std::move(data));

		++i;
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
