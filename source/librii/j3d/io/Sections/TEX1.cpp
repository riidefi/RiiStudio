#include "../Sections.hpp"

namespace librii::j3d {

Result<void> Tex::transfer(rsl::SafeReader& stream) {
  auto block = stream.scoped("Tex");

  mFormat = TRY(stream.Enum8<librii::gx::TextureFormat>());
  transparency = stream.Enum8<JUTTransparency>().value_or(
      static_cast<JUTTransparency>(0xCC));
  mWidth = TRY(stream.U16());
  mHeight = TRY(stream.U16());
  mWrapU = TRY(stream.Enum8<gx::TextureWrapMode>());
  mWrapV = TRY(stream.Enum8<gx::TextureWrapMode>());
  stream.getUnsafe().skip(1);
  mPaletteFormat = TRY(stream.U8());
  nPalette = TRY(stream.U16());
  ofsPalette = TRY(stream.U32());
  // EXPECT(ofsPalette == 0, "No palette support..");
  bMipMap = TRY(stream.U8());
  bEdgeLod = TRY(stream.U8());
  bBiasClamp = TRY(stream.U8());
  mMaxAniso = TRY(stream.Enum8<librii::gx::AnisotropyLevel>());
  mMinFilter = TRY(stream.Enum8<librii::gx::TextureFilter>());
  mMagFilter = TRY(stream.Enum8<librii::gx::TextureFilter>());
  mMinLod = TRY(stream.S8());
  mMaxLod = TRY(stream.S8());
  mMipmapLevel = TRY(stream.U8());
  if (mMipmapLevel == 0) {
    stream.getUnsafe().warnAt("Invalid LOD: 0. Valid range: [1, 6]",
                              stream.tell() - 1, stream.tell());
    mMipmapLevel = 1;
  }
  // assert(mMipmapLevel);
  stream.getUnsafe().skip(1);
  mLodBias = TRY(stream.S16());
  ofsTex = TRY(stream.U32());

  EXPECT(stream.tell() - block.start == 0x20);
  return {};
}
void Tex::write(oishii::Writer& stream) const {
  oishii::DebugExpectSized dbg(stream, 0x20 - 4);

  stream.write<u8>(static_cast<u8>(mFormat));
  stream.write<u8>(static_cast<u8>(transparency));
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
Tex::Tex(const librii::j3d::TextureData& data,
         const libcube::GCMaterialData::SamplerData& sampl) {
  mFormat = data.mFormat;
  transparency = data.transparency;
  mWidth = data.mWidth;
  mHeight = data.mHeight;
  mWrapU = sampl.mWrapU;
  mWrapV = sampl.mWrapV;
  mPaletteFormat = data.mPaletteFormat;
  nPalette = data.nPalette;
  ofsPalette = 0;
  bMipMap = sampl.mMinFilter != librii::gx::TextureFilter::Linear &&
            sampl.mMinFilter != librii::gx::TextureFilter::Near;
  bEdgeLod = sampl.bEdgeLod;
  bBiasClamp = sampl.bBiasClamp;
  mMaxAniso = sampl.mMaxAniso;
  mMinFilter = sampl.mMinFilter;
  mMagFilter = sampl.mMagFilter;
  mMinLod = data.mMinLod;
  mMaxLod = data.mMaxLod;
  mMipmapLevel = data.mImageCount;
  mLodBias = roundf(sampl.mLodBias * 100.0f);
  ofsTex = -1;
}
Result<void> readTEX1(BMDOutputContext& ctx) {
  auto& reader = ctx.reader;
  if (!enterSection(ctx, 'TEX1'))
    return std::unexpected(
        "Failed to find TEX1 section -- this may not be an error");

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

  ctx.mTexCache.clear();

  struct RawTexture {
    TextureData data;
    u32 absolute_file_offset;
    u32 byte_size;

    RawTexture() { data.mData.resize(0); }
  };

  std::vector<RawTexture> texRaw;

  reader.skip(ofsHeaders);
  for (int i = 0; i < size; ++i) {
    Tex tex;
    tex.btiId = i;
    rsl::SafeReader safe(reader);
    TRY(tex.transfer(safe));
    printf(":: (%d) %s\n", i, nameTable[i].c_str());

    if (librii::gx::IsPaletteFormat(tex.mFormat)) {
      return std::unexpected("Texture \"" + nameTable[i] +
                             "\" uses a paletted format which is unsupported.");
    }

    for (auto& mat : ctx.mdl.materials) {
      for (int k = 0; k < mat.samplers.size(); ++k) {
        auto& samp = mat.samplers[k];
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
    for (auto& samp : ctx.mMatCache.samplers) {
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
    ctx.mTexCache.push_back(tex);
    auto& inf = texRaw.emplace_back();
    auto& data = inf.data;

    data.mName = nameTable[i];
    data.mFormat = tex.mFormat;
    data.mWidth = tex.mWidth;
    data.mHeight = tex.mHeight;
    data.mPaletteFormat = tex.mPaletteFormat;
    data.nPalette = tex.nPalette;
    data.ofsPalette = tex.ofsPalette;
    data.mMinLod = tex.mMinLod;
    data.mMaxLod = tex.mMaxLod;
    data.mImageCount = tex.mMipmapLevel;

    inf.absolute_file_offset = g.start + ofsHeaders + i * 32 + tex.ofsTex;
    inf.byte_size = librii::gx::computeImageSize(tex.mWidth, tex.mHeight,
                                                 tex.mFormat, tex.mMipmapLevel);
  }

  // Deduplicate and read.
  // Assumption: Data will not be spliced

  struct UniqueImage {
    u32 offset;
    int bti_index; // This may be the first of many

    UniqueImage(u32 ofs, int idx) : offset(ofs), bti_index(idx) {}
  };
  std::vector<UniqueImage> uniques;
  for (int i = 0; i < size; ++i) {
    const auto found =
        std::find_if(uniques.begin(), uniques.end(), [&](const auto& it) {
          return it.offset == texRaw[i].absolute_file_offset;
        });
    if (found == uniques.end())
      uniques.emplace_back(texRaw[i].absolute_file_offset, i);
  }

  int i = 0;
  for (const auto& it : uniques) {
    auto& texpair = texRaw[it.bti_index];
    reader.readBuffer(texpair.data.mData, texpair.byte_size,
                      texpair.absolute_file_offset);
    ctx.mdl.textures.emplace_back() = texpair.data;

    ++i;
  }

  for (auto& mat : ctx.mdl.materials) {
    for (int k = 0; k < mat.samplers.size(); ++k) {
      auto& samp = mat.samplers[k];
      if (samp.mTexture.empty()) {
        printf("Material %s: Sampler %u is invalid.\n", mat.name.c_str(),
               (u32)i);
        EXPECT(!samp.mTexture.empty());
      }
    }
  }

  return {};
}
struct TEX1Node final : public oishii::Node {
  TEX1Node(const BMDExportContext& model) : mModel(model) {
    mId = "TEX1";
    mLinkingRestriction.alignment = 32;
  }

  struct TexNames : public oishii::Node {
    TexNames(const BMDExportContext& mdl) : mMdl(mdl) {
      mId = "TexNames";
      getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = 4;
    }

    Result write(oishii::Writer& writer) const noexcept {
      std::vector<std::string> names;

      for (int i = 0; i < mMdl.mTexCache.size(); ++i)
        names.push_back(mMdl.mdl.textures[mMdl.mTexCache[i].btiId].mName);
      for (int i = 0; i < names.size(); ++i) {
        printf(":: (%i) %s\n", i, names[i].c_str());
      }
      writeNameTable(writer, names);
      writer.alignTo(32);
      return {};
    }

    const BMDExportContext mMdl;
  };

  struct TexHeaders : public oishii::Node {
    TexHeaders(const BMDExportContext& mdl) : mMdl(mdl) {
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
      for (auto& tex : mMdl.mTexCache) {
        d.addNode(std::make_unique<TexHeaderEntryLink>(tex, id++, tex.btiId));
      }
      return {};
    }

    const BMDExportContext mMdl;
  };
  struct TexEntry : public oishii::Node {
    TexEntry(const J3dModel& mdl, u32 texIdx) : mMdl(mdl), mIdx(texIdx) {
      mId = std::to_string(texIdx);
      getLinkingRestriction().setLeaf();
      getLinkingRestriction().alignment = 32;
    }

    Result write(oishii::Writer& writer) const noexcept {
      const auto& tex = mMdl.textures[mIdx];
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

    const J3dModel& mMdl;
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

    d.addNode(std::make_unique<TexHeaders>(mModel));

    for (int i = 0; i < mModel.mdl.textures.size(); ++i)
      d.addNode(std::make_unique<TexEntry>(mModel.mdl, i));

    d.addNode(std::make_unique<TexNames>(mModel));

    return {};
  }

private:
  const BMDExportContext mModel;
};

std::unique_ptr<oishii::Node> makeTEX1Node(BMDExportContext& ctx) {
  return std::make_unique<TEX1Node>(ctx);
}

} // namespace librii::j3d
