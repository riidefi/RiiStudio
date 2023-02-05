#include <LibBadUIFramework/Node2.hpp>
#include <core/common.h>

#include <oishii/reader/binary_reader.hxx>
#include <oishii/writer/binary_writer.hxx>
#include <oishii/writer/linker.hxx>

#include <plugins/j3d/Scene.hpp>

#include "Sections.hpp"

#include <core/util/timestamp.hpp>
#include <librii/gx/validate/MaterialValidate.hpp>
#include <plugins/j3d/J3dIo.hpp>
#include <vendor/magic_enum/magic_enum.hpp>

#include <librii/j3d/J3dIo.hpp>

IMPORT_STD;

bool gTestMode = false;

namespace librii::j3d {

using namespace libcube;

Result<void> processCollectionForWrite(BMDExportContext& collection);

struct BMDFile : public oishii::Node {
  static const char* getNameId() { return "JSystem Binary Model Data"; }

  Result write(oishii::Writer& writer) const noexcept {
    // Hack
    writer.write<u32, oishii::EndianSelect::Big>('J3D2');
    writer.write<u32, oishii::EndianSelect::Big>(bBDL ? 'bdl4' : 'bmd3');

    // Filesize
    writer.writeLink<s32>(oishii::Link{
        oishii::Hook(*this), oishii::Hook(*this, oishii::Hook::EndOfChildren)});

    // 8 sections
    writer.write<u32>(bBDL ? 9 : 8);

    if (gTestMode) {
      // SubVeRsion
      writer.write<u32, oishii::EndianSelect::Big>('SVR3');
      for (int i = 0; i < 3; ++i)
        writer.write<u32>(-1);
    } else {
      std::string author = VERSION_SHORT;
      for (char c : author)
        writer.write<char>(c);
    }

    return {};
  }
  Result gatherChildren(oishii::Node::NodeDelegate& ctx) const {
    pExp = std::make_unique<BMDExportContext>(BMDExportContext{*mCollection});
    auto& exp = *pExp;
    processCollectionForWrite(exp);

    auto addNode = [&](std::unique_ptr<oishii::Node> node) {
      node->getLinkingRestriction().alignment = 32;
      ctx.addNode(std::move(node));
    };

    addNode(makeINF1Node(exp));
    addNode(makeVTX1Node(exp));
    addNode(makeEVP1Node(exp));
    addNode(makeDRW1Node(exp));
    addNode(makeJNT1Node(exp));
    addNode(makeSHP1Node(exp));
    addNode(makeMAT3Node(exp));
    if (bBDL)
      addNode(makeMDL3Node(exp));
    addNode(makeTEX1Node(exp));
    return {};
  }

  std::unique_ptr<J3dModel> mCollection;
  bool bBDL = true;
  bool bMimic = true;

  mutable std::unique_ptr<BMDExportContext> pExp;
};
void BMD_Pad(char* dst, u32 len) {
  const char* pad_str = "This is padding data to alignment.....";
  if (!gTestMode) {
    pad_str = RII_TIME_STAMP;
  }

  if (len < strlen(pad_str))
    memcpy(dst, pad_str, len);
  else
    memset(dst, 0, len);
}

Result<void>
processModelForWrite(BMDExportContext& model,
                     const std::map<std::string, u32>& texNameMap) {
  auto& texCache = model.mTexCache;
  auto& matCache = model.mMatCache;
  texCache.clear();
  matCache.clear();

  libcube::GCMaterialData::SamplerData samp;

  for (size_t i = 0; i < model.mdl.textures.size(); ++i) {
    auto& tex = model.mdl.textures[i];
    std::vector<libcube::GCMaterialData::SamplerData*> its;
    for (auto& mat : model.mdl.materials) {
      for (int i = 0; i < mat.samplers.size(); ++i) {
        auto& csamp = mat.samplers[i];
        EXPECT(!csamp.mTexture.empty());

        if (csamp.mTexture == tex.mName) {
          its.push_back(&csamp);
        }
      }
    }
    // Include unused textures
    if (its.empty()) {
      its.push_back(nullptr);
    }
    for (auto* it : its) {
      if (it != nullptr) {
        samp = *it;
      }
      Tex tmp(tex, samp);
      tmp.btiId = i;

      auto found = std::find(texCache.begin(), texCache.end(), tmp);
      if (found == texCache.end()) {
        texCache.push_back(tmp);
        if (it)
          it->btiId = texCache.size() - 1;
      } else {
        if (it)
          it->btiId = found - texCache.begin();
      }
    }
  }

  for (auto& mat : model.mdl.materials) {
    matCache.propagate(mat);
  }

  return {};
}

// Recompute cache
Result<void> processCollectionForWrite(BMDExportContext& collection) {
  std::map<std::string, u32> texNameMap;
  for (int i = 0; i < collection.mdl.textures.size(); ++i) {
    texNameMap[collection.mdl.textures[i].mName] = i;
  }

  return processModelForWrite(collection, texNameMap);
}

Result<void> detailWriteBMD(const J3dModel& model_, oishii::Writer& writer) {
  auto model = std::make_unique<J3dModel>(model_);
  oishii::Linker linker;

  auto bmd = std::make_unique<BMDFile>();
  bmd->bBDL = model->isBDL;
  bmd->bMimic = true;
  bmd->mCollection = std::move(model);

  linker.mUserPad = &BMD_Pad;
  writer.mUserPad = &BMD_Pad;

  // writer.add_bp(0x37b2c, 4);

  linker.gather(std::move(bmd), "");
  TRY(linker.write(writer));
  return {};
}

Result<void> detailReadBMD(J3dModel& mdl, oishii::BinaryReader& reader,
                           kpi::LightIOTransaction& transaction) {
  BMDOutputContext ctx{mdl, {}, {}, reader, transaction};

  reader.setEndian(std::endian::big);
  const auto magic = reader.read<u32>();
  if (magic == 'J3D2') {
    // Big endian
  } else if (magic == '2D3J') {
    reader.setEndian(std::endian::little);
  } else {
    return std::unexpected(std::format("Unexpected file magic: 0x{:x}", magic));
  }

  mdl.isBDL = false;

  u32 bmdVer = reader.read<u32>();
  if (bmdVer == 'bmd3') {
  } else if (bmdVer == 'bdl4') {
#ifndef BUILD_DIST
    // mdl.isBDL = true;
#endif
  } else {
    reader.signalInvalidityLast<u32, oishii::MagicInvalidity<'bmd3'>>();
    return std::unexpected(
        std::format("Unsupporte BMD version 0x{:x}", bmdVer));
  }

  // TODO: Validate file size.
  // const auto fileSize =
  reader.read<u32>();
  const auto sec_count = reader.read<u32>();

  // Skip SVR3 header
  reader.seek<oishii::Whence::Current>(16);

  // Scan the file
  ctx.mSections.clear();
  for (u32 i = 0; i < sec_count; ++i) {
    const u32 secType = ctx.reader.read<u32>();
    const u32 secSize = ctx.reader.read<u32>();

    {
      oishii::JumpOut g(ctx.reader, secSize - 8);
      switch (secType) {
      case 'INF1':
      case 'VTX1':
      case 'EVP1':
      case 'DRW1':
      case 'JNT1':
      case 'SHP1':
      case 'MAT3':
      case 'MDL3':
      case 'TEX1':
        ctx.mSections[secType] = {ctx.reader.tell(), secSize};
        break;
      default:
        ctx.reader.warnAt("Unexpected section type.", ctx.reader.tell() - 8,
                          ctx.reader.tell() - 4);
        break;
      }
    }
  }
  // Read vertex data
  TRY(readVTX1(ctx));

  // Read joints
  TRY(readJNT1(ctx));

  // Read envelopes and draw matrices
  TRY(readEVP1DRW1(ctx));

  // Read shapes
  TRY(readSHP1(ctx));

  for (const auto& e : ctx.mVertexBufferMaxIndices) {
    switch (e.first) {
    case gx::VertexBufferAttribute::Position:
      if (ctx.mdl.vertexData.pos.mData.size() != e.second + 1) {
        rsl::trace(
            "The position vertex buffer currently has {} greedily-claimed "
            "entries due to 32B padding; {} are used.",
            ctx.mdl.vertexData.pos.mData.size(), e.second + 1);
        ctx.mdl.vertexData.pos.mData.resize(e.second + 1);
      }
      break;
    case gx::VertexBufferAttribute::Color0:
    case gx::VertexBufferAttribute::Color1: {
      auto& buf =
          ctx.mdl.vertexData
              .color[(int)e.first - (int)gx::VertexBufferAttribute::Color0];
      if (buf.mData.size() != e.second + 1) {
        rsl::trace("The color buffer currently has {} greedily-claimed entries "
                   "due to 32B padding; {} are used.",
                   buf.mData.size(), e.second + 1);
        buf.mData.resize(e.second + 1);
      }
      break;
    }
    case gx::VertexBufferAttribute::Normal: {
      auto& buf = ctx.mdl.vertexData.norm;
      if (buf.mData.size() != e.second + 1) {
        rsl::trace(
            "The normal buffer currently has {} greedily-claimed entries "
            "due to 32B padding; {} are used.",
            buf.mData.size(), e.second + 1);
        buf.mData.resize(e.second + 1);
      }
      break;
    }
    case gx::VertexBufferAttribute::TexCoord0:
    case gx::VertexBufferAttribute::TexCoord1:
    case gx::VertexBufferAttribute::TexCoord2:
    case gx::VertexBufferAttribute::TexCoord3:
    case gx::VertexBufferAttribute::TexCoord4:
    case gx::VertexBufferAttribute::TexCoord5:
    case gx::VertexBufferAttribute::TexCoord6:
    case gx::VertexBufferAttribute::TexCoord7: {
      auto& buf =
          ctx.mdl.vertexData
              .uv[(int)e.first - (int)gx::VertexBufferAttribute::TexCoord0];
      if (buf.mData.size() != e.second + 1) {
        rsl::trace(
            "The UV buffer currently has {} greedily-claimed entries due "
            "to 32B padding; {} are used.",
            buf.mData.size(), e.second + 1);
        buf.mData.resize(e.second + 1);
      }
      break;
    }
      //      // clang-format off
      //    case
      //    (gx::VertexBufferAttribute)(int)gx::VertexAttribute::PositionNormalMatrixIndex:
      //      // clang-format on
      //      break;
    default:
      return std::unexpected(std::format(
          "Unsupported VertexBufAttribute {} ({})", static_cast<int>(e.first),
          magic_enum::enum_name(e.first)));
    }
  }

  // Read materials
  TRY(readMAT3(ctx));

  // fixup identity matrix optimization for editor
  for (auto& mat : ctx.mdl.materials) {
    if (mat.texGens.size() != mat.texMatrices.size())
      continue;

    // Compute a used bitmap. 8 max texgens, 10 max texmatrices
    u32 used = 0;
    for (int i = 0; i < mat.texGens.size(); ++i) {
      if (auto idx = mat.texGens[i].getMatrixIndex(); idx > 0)
        used |= (1 << idx);
    }

    for (int i = 0; i < mat.texGens.size(); ++i) {
      auto& tg = mat.texGens[i];

      if (tg.isIdentityMatrix() && (used & (1 << i)) == 0) {
        EXPECT(mat.texMatrices[i].isIdentity());
        tg.setMatrixIndex(i);
      }
    }

    librii::gx::expandSharedTexGens(mat);
  }

  // Read TEX1
  TRY(readTEX1(ctx));

  // Read INF1
  TRY(readINF1(ctx));

  // Read MDL3
  return {};
}

Result<librii::j3d::J3dModel>
librii::j3d::J3dModel::read(oishii::BinaryReader& reader,
                            kpi::LightIOTransaction& tx) {
  librii::j3d::J3dModel out;
  TRY(detailReadBMD(out, reader, tx));
  return out;
}
Result<void> librii::j3d::J3dModel::write(oishii::Writer& writer) {
  return detailWriteBMD(*this, writer);
}

static gx::TexCoordGen postTexGen(const gx::TexCoordGen& gen) {
  return gx::TexCoordGen{// gen.id,
                         gen.func, gen.sourceParam,
                         static_cast<gx::TexMatrix>(gen.postMatrix), false,
                         gen.postMatrix};
}
void MatCache::propagate(librii::j3d::MaterialData& mat) {
  Indirect _ind(mat);
  indirectInfos.push_back(mat); // one per mat
  update_section(cullModes, mat.cullMode);
  for (int i = 0; i < mat.chanData.size(); ++i) {
    auto& chan = mat.chanData[i];
    update_section(matColors, chan.matColor);
    update_section(ambColors, chan.ambColor);
  }
  update_section(nColorChan, static_cast<u8>(mat.chanData.size()));
  update_section_multi(colorChans, mat.colorChanControls);
  update_section_multi(lightColors, mat.lightColors);
  update_section(nTexGens, static_cast<u8>(mat.texGens.size()));
  auto tgs = mat.texGens;
  for (auto& tg : tgs) {
    if (auto mtx = tg.getMatrixIndex();
        mtx > 0 && mat.texMatrices[mtx].isIdentity())
      tg.matrix = gx::TexMatrix::Identity;
  }
  update_section_multi(texGens, tgs);

  for (int i = 0; i < mat.texGens.size(); ++i) {
    if (mat.texGens[i].postMatrix != gx::PostTexMatrix::Identity) {
      update_section(posTexGens, postTexGen(mat.texGens[i]));
    }
  }

  for (int i = 0; i < mat.texMatrices.size(); ++i) {
    update_section(texMatrices, mat.texMatrices[i]);
  }
  for (int i = 0; i < mat.postTexMatrices.size(); ++i) {
    update_section(postTexMatrices, mat.postTexMatrices[i]);
  }

  for (int i = 0; i < mat.samplers.size(); ++i) {
    // We have already condensed samplers by this point. Only the ID matters.
    // MaterialData::J3DSamplerData tmp;
    // tmp.btiId =
    // reinterpret_cast<MaterialData::J3DSamplerData*>(mat.samplers[i].get())->btiId;
    // update_section(samplers, tmp);
    update_section(samplers, mat.samplers[i]);
  }
  for (auto& stage : mat.mStages) {
    TevOrder order;
    order.rasOrder = stage.rasOrder;
    order.texCoord = stage.texCoord;
    order.texMap = stage.texMap;

    update_section(orders, order);

    SwapSel swap;
    swap.colorChanSel = stage.rasSwap;
    swap.texSel = stage.texMapSwap;

    update_section(swapModes, swap);

    librii::gx::TevStage tmp;
    tmp.colorStage = stage.colorStage;
    tmp.colorStage.constantSelection = gx::TevKColorSel::k0;
    tmp.alphaStage = stage.alphaStage;
    tmp.alphaStage.constantSelection = gx::TevKAlphaSel::k0_a;

    update_section(tevStages, tmp);
  }
  update_section_multi(tevColors, mat.tevColors);
  update_section_multi(konstColors, mat.tevKonstColors);
  update_section(nTevStages, static_cast<u8>(mat.mStages.size()));

  update_section_multi(swapTables, mat.mSwapTable);
  update_section(fogs, mat.fogInfo);
  update_section(alphaComparisons, mat.alphaCompare);
  update_section(blendModes, mat.blendMode);
  update_section(zModes, mat.zMode);
  update_section(zCompLocs, static_cast<u8>(mat.earlyZComparison));
  update_section(dithers, static_cast<u8>(mat.dither));
  update_section(nbtScales, mat.nbtScale);
}

} // namespace librii::j3d
