#include "../Sections.hpp"
#include <map>
#include <string.h>
#include <vendor/ogc/texture.h>

namespace riistudio::j3d {

void Tex::transfer(oishii::BinaryReader& stream) {
  oishii::DebugExpectSized dbg(stream, 0x20);

  mFormat = static_cast<libcube::gx::TextureFormat>(stream.read<u8>());
  bTransparent = stream.read<u8>();
  mWidth = stream.read<u16>();
  mHeight = stream.read<u16>();
  mWrapU = static_cast<libcube::gx::TextureWrapMode>(stream.read<u8>());
  mWrapV = static_cast<libcube::gx::TextureWrapMode>(stream.read<u8>());
  stream.skip(1);
  stream.transfer(mPaletteFormat);
  stream.transfer(nPalette);
  stream.transfer(ofsPalette);
  // assert(ofsPalette == 0 && "No palette support..");
  stream.transfer(bMipMap);
  stream.transfer(bEdgeLod);
  stream.transfer(bBiasClamp);
  mMaxAniso = static_cast<libcube::gx::AnisotropyLevel>(stream.read<u8>());
  mMinFilter = static_cast<libcube::gx::TextureFilter>(stream.read<u8>());
  mMagFilter = static_cast<libcube::gx::TextureFilter>(stream.read<u8>());
  stream.transfer<s8>(mMinLod);
  stream.transfer<s8>(mMaxLod);
  stream.transfer<u8>(mMipmapLevel);
  if (mMipmapLevel == 0) {
    stream.warnAt("Invalid LOD: 0. Valid range: [1, 6]", stream.tell() - 1,
                  stream.tell());
    mMipmapLevel = 1;
  }
  // assert(mMipmapLevel);
  stream.skip(1);
  stream.transfer(mLodBias);
  stream.transfer(ofsTex);
}
void Tex::write(oishii::Writer& stream) const {
  oishii::DebugExpectSized dbg(stream, 0x20 - 4);

  stream.write<u8>(static_cast<u8>(mFormat));
  stream.write<u8>(bTransparent);
  stream.write<u16>(mWidth);
  stream.write<u16>(mHeight);
  stream.write<u8>(static_cast<u8>(mWrapU));
  stream.write<u8>(static_cast<u8>(mWrapV));
  stream.skip(1);
  stream.write<u8>(mPaletteFormat);
  stream.write<u16>(nPalette);
  stream.write<u32>(0);
  // stream.transfer(ofsPalette);
  stream.write<u8>(bMipMap);
  stream.write<u8>(bEdgeLod);
  stream.write<u8>(bBiasClamp);
  stream.write<u8>(static_cast<u8>(mMaxAniso));
  stream.write<u8>(static_cast<u8>(mMinFilter));
  stream.write<u8>(static_cast<u8>(mMagFilter));
  stream.write<s8>(mMinLod);
  stream.write<s8>(mMaxLod);
  stream.write<u8>(mMipmapLevel);
  stream.skip(1);
  stream.write<s16>(mLodBias);
  // stream.transfer(ofsTex);
}
Tex::Tex(const Texture& data,
         const libcube::GCMaterialData::SamplerData& sampl) {
  mFormat = static_cast<libcube::gx::TextureFormat>(data.mFormat);
  bTransparent = data.bTransparent;
  mWidth = data.mWidth;
  mHeight = data.mHeight;
  mWrapU = sampl.mWrapU;
  mWrapV = sampl.mWrapV;
  mPaletteFormat = data.mPaletteFormat;
  nPalette = data.nPalette;
  ofsPalette = 0;
  bMipMap = sampl.mMinFilter != libcube::gx::TextureFilter::linear &&
            sampl.mMinFilter != libcube::gx::TextureFilter::near;
  bEdgeLod = sampl.bEdgeLod;
  bBiasClamp = sampl.bBiasClamp;
  mMaxAniso = sampl.mMaxAniso;
  mMinFilter = sampl.mMinFilter;
  mMagFilter = sampl.mMagFilter;
  mMinLod = data.mMinLod;
  mMaxLod = data.mMaxLod;
  mMipmapLevel = data.mMipmapLevel;
  mLodBias = roundf(sampl.mLodBias * 100.0f);
  ofsTex = -1;
}
void readTEX1(BMDOutputContext& ctx) {
  auto& reader = ctx.reader;
  if (!enterSection(ctx, 'TEX1'))
    return;

  ScopedSection g(ctx.reader, "Textures");

  u16 size = reader.read<u16>();
  reader.read<u16>();

  const auto [ofsHeaders, ofsNameTable] = reader.readX<s32, 2>();
  reader.seekSet(g.start);

  std::vector<std::string> nameTable;
  {
    oishii::Jump<oishii::Whence::Current> j(reader, ofsNameTable);
    nameTable = readNameTable(reader);
  }

  ctx.mdl.mTexCache.clear();

  std::vector<std::pair<std::unique_ptr<Texture>, std::pair<u32, u32>>> texRaw;

  reader.skip(ofsHeaders);
  for (int i = 0; i < size; ++i) {
    Tex tex;
    tex.btiId = i;
    tex.transfer(reader);
    for (auto& mat : ctx.mdl.getMaterials()) {
      for (int k = 0; k < mat.samplers.size(); ++k) {
        auto& samp = (Material::J3DSamplerData&)*mat.samplers[k].get();
        if (samp.btiId == i) {
          samp.mTexture = nameTable[i];
          samp.mWrapU = tex.mWrapU;
          samp.mWrapV = tex.mWrapV;
          // samp.bMipMap = tex.bMipMap;
          samp.bEdgeLod = tex.bEdgeLod;
          samp.bBiasClamp = tex.bBiasClamp;
          samp.mMaxAniso = tex.mMaxAniso;
          samp.mMinFilter = tex.mMinFilter;
          samp.mMagFilter = tex.mMagFilter;
          samp.mLodBias = static_cast<f32>(tex.mLodBias) / 100.0f;
        }
      }
    }
    for (auto& samp : ctx.mdl.mMatCache.samplers) {
      if (samp.btiId == i) {
        samp.mTexture = nameTable[i];
        samp.mWrapU = tex.mWrapU;
        samp.mWrapV = tex.mWrapV;
        // samp.bMipMap = tex.bMipMap;
        samp.bEdgeLod = tex.bEdgeLod;
        samp.bBiasClamp = tex.bBiasClamp;
        samp.mMaxAniso = tex.mMaxAniso;
        samp.mMinFilter = tex.mMinFilter;
        samp.mMagFilter = tex.mMagFilter;
        samp.mLodBias = static_cast<f32>(tex.mLodBias) / 100.0f;
      }
    }
    ctx.mdl.mTexCache.push_back(tex);
    auto& inf = texRaw.emplace_back(std::make_unique<Texture>(),
                                    std::pair<u32, u32>{0, 0});
    auto& data = *inf.first.get();

    data.mName = nameTable[i];
    data.mFormat = static_cast<u8>(tex.mFormat);
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
    inf.second.second =
        GetTexBufferSize(tex.mWidth, tex.mHeight, static_cast<u32>(tex.mFormat),
                         true, tex.mMipmapLevel);
  }

  // Deduplicate and read.
  // Assumption: Data will not be spliced

  std::vector<std::pair<u32, int>> uniques; // ofs : index
  for (int i = 0; i < size; ++i) {
    const auto found =
        std::find_if(uniques.begin(), uniques.end(), [&](const auto& it) {
          return it.first == texRaw[i].second.first;
        });
    if (found == uniques.end())
      uniques.emplace_back(texRaw[i].second.first, i);
  }

  int i = 0;
  for (const auto& it : uniques) {
    auto& texpair = texRaw[it.second];
    const auto [ofs, size] = texpair.second;
    std::unique_ptr<Texture> data = std::move(texpair.first);
	reader.readBuffer(data->mData, size, ofs);
    ctx.col.getTextures().add() = *data.get();

    ++i;
  }

  for (auto& mat : ctx.mdl.getMaterials()) {
    for (int k = 0; k < mat.samplers.size(); ++k) {
      auto& samp = mat.samplers[k];
      if (samp->mTexture.empty()) {
        printf("Material %s: Sampler %u is invalid.\n", mat.getName().c_str(),
               (u32)i);
        assert(!samp->mTexture.empty());
      }
    }
  }
}
struct TEX1Node final : public oishii::Node {
  TEX1Node(const Model& model, const Collection& col)
      : mModel(model), mCol(col) {
    mId = "TEX1";
    mLinkingRestriction.alignment = 32;
  }

  struct TexNames : public oishii::Node {
    TexNames(const Model& mdl, const Collection& col) : mMdl(mdl), mCol(col) {
      mId = "TexNames";
      getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = 4;
    }

    Result write(oishii::Writer& writer) const noexcept {
      std::vector<std::string> names;

      for (int i = 0; i < mMdl.mTexCache.size(); ++i)
        names.push_back(mCol.getTextures()[mMdl.mTexCache[i].btiId].getName());
      writeNameTable(writer, names);
      writer.alignTo(32);
      return {};
    }

    const Model& mMdl;
    const Collection& mCol;
  };

  struct TexHeaders : public oishii::Node {
    TexHeaders(const Model& mdl, const Collection& col) : mMdl(mdl), mCol(col) {
      mId = "TexHeaders";
      getLinkingRestriction().alignment = 32;
    }

    struct TexHeaderEntryLink : public oishii::Node {
      TexHeaderEntryLink(const Tex& _tex, u32 id, u32 _btiId)
          : tex(_tex), btiId(_btiId) {
        mId = std::to_string(id);
        getLinkingRestriction().setLeaf();
        getLinkingRestriction().alignment = 4;
      }
      Result write(oishii::Writer& writer) const noexcept {
        tex.write(writer);
        writer.writeLink<s32>(*this, "TEX1::" + std::to_string(btiId));
        return {};
      }
      const Tex& tex;
      u32 btiId;
    };

    Result write(oishii::Writer& writer) const noexcept { return {}; }

    Result gatherChildren(NodeDelegate& d) const noexcept override {
      u32 id = 0;
      for (auto& tex : mMdl.mTexCache)
        d.addNode(std::make_unique<TexHeaderEntryLink>(tex, id++, tex.btiId));
      return {};
    }

    const Model& mMdl;
    const Collection& mCol;
  };
  struct TexEntry : public oishii::Node {
    TexEntry(const Model& mdl, const Collection& col, u32 texIdx)
        : mMdl(mdl), mCol(col), mIdx(texIdx) {
      mId = std::to_string(texIdx);
      getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = 32;
    }

    Result write(oishii::Writer& writer) const noexcept {
      const auto& tex = mCol.getTextures()[mIdx];
      const auto before = writer.tell();

      writer.skip(tex.mData.size() - 1);
      writer.write<u8>(0);
      writer.skip(-tex.mData.size());

      // memcpy_s(writer.getDataBlockStart() + before, writer.endpos(),
      // tex.mData.data(), tex.mData.size());
      memcpy(writer.getDataBlockStart() + before, tex.mData.data(),
             tex.mData.size());
      writer.skip(tex.mData.size());
      return {};
    }

    const Model& mMdl;
    const Collection& mCol;
    const u32 mIdx;
  };
  Result write(oishii::Writer& writer) const noexcept override {
    writer.write<u32, oishii::EndianSelect::Big>('TEX1');
    writer.writeLink<s32>({*this}, {*this, oishii::Hook::EndOfChildren});

    writer.write<u16>((u16)mModel.mTexCache.size());
    writer.write<u16>(-1);

    writer.writeLink<s32>(oishii::Hook(*this), oishii::Hook("TexHeaders"));
    writer.writeLink<s32>(oishii::Hook(*this), oishii::Hook("TexNames"));

    return eResult::Success;
  }

  Result gatherChildren(NodeDelegate& d) const noexcept override {

    d.addNode(std::make_unique<TexHeaders>(mModel, mCol));

    for (int i = 0; i < mCol.getTextures().size(); ++i)
      d.addNode(std::make_unique<TexEntry>(mModel, mCol, i));

    d.addNode(std::make_unique<TexNames>(mModel, mCol));

    return {};
  }

private:
  const Model& mModel;
  const Collection& mCol;
};

std::unique_ptr<oishii::Node> makeTEX1Node(BMDExportContext& ctx) {
  return std::make_unique<TEX1Node>(ctx.mdl, ctx.col);
}

} // namespace riistudio::j3d
