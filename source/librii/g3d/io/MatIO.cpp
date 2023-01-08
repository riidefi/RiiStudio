#include "MatIO.hpp"
#include <librii/gpu/DLBuilder.hpp>
#include <rsl/Ranges.hpp>

namespace librii::g3d {

u32 BuildTexMatrixFlags(const librii::gx::GCMaterialData::TexMatrix& mtx) {
  // TODO: Float equality
  const u32 identity_scale = mtx.scale == glm::vec2{1.0f, 1.0f};
  const u32 identity_rotate = mtx.rotate == 0.0f;
  const u32 identity_translate = mtx.translate == glm::vec2{0.0f, 0.0f};
  // printf("identity_scale: %u, identity_rotate: %u,
  // identity_translate: "
  //        "%u\n",
  //        identity_scale, identity_rotate, identity_translate);
  return 1 | (identity_scale << 1) | (identity_rotate << 2) |
         (identity_translate << 3);
}

Result<void> BinaryMatDL::write(oishii::Writer& writer) const {
  MAYBE_UNUSED const auto dl_start = writer.tell();
  DebugReport("Mat dl start: %x\n", (unsigned)writer.tell());
  librii::gpu::DLBuilder dl(writer);
  {
    dl.setAlphaCompare(alphaCompare);
    dl.setZMode(zMode);
    dl.setBlendMode(blendMode);
    dl.setDstAlpha(dstAlpha.enabled, dstAlpha.alpha);
    dl.align();
  }
  EXPECT(writer.tell() - dl_start == 0x20);
  {
    for (int i = 0; i < 3; ++i)
      dl.setTevColor(i + 1, tevColors[i]);
    dl.align(); // pad 4
    for (int i = 0; i < 4; ++i)
      dl.setTevKColor(i, tevKonstColors[i]);
    dl.align(); // 24
  }
  EXPECT(writer.tell() - dl_start == 0xa0);
  {
    dl.setIndTexCoordScale(0, scales[0], scales[1]);
    dl.setIndTexCoordScale(2, scales[2], scales[3]);

    const auto write_ind_mtx = [&](std::size_t i) {
      if (indMatrices.size() > i)
        dl.setIndTexMtx(i, indMatrices[i]);
      else
        writer.skip(5 * 3);
    };
    write_ind_mtx(0);
    dl.align(); // 7
    write_ind_mtx(1);
    write_ind_mtx(2);
    dl.align();
  }
  EXPECT(writer.tell() - dl_start == 0xe0);
  {
    for (int i = 0; i < texGens.size(); ++i)
      dl.setTexCoordGen(i, texGens[i]);
    writer.skip(18 * (8 - texGens.size()));
    dl.align();
  }
  EXPECT(writer.tell() - dl_start == 0x180);
  return {};
}
Result<void> BinaryMatDL::parse(oishii::BinaryReader& reader, u32 numIndStages,
                                u32 numTexGens) {
  gx::LowLevelGxMaterial mat;
  librii::gpu::QDisplayListMaterialHandler matHandler(mat);

  // Pixel data
  TRY(librii::gpu::RunDisplayList(reader, matHandler, 32));
  alphaCompare = matHandler.mGpuMat.mPixel.mAlphaCompare;
  zMode = matHandler.mGpuMat.mPixel.mZMode;
  blendMode = matHandler.mGpuMat.mPixel.mBlendMode;
  dstAlpha = matHandler.mGpuMat.mPixel.mDstAlpha;

  // Uniform data
  TRY(librii::gpu::RunDisplayList(reader, matHandler, 128));
  for (int i = 0; i < 3; ++i) {
    tevColors[i] = matHandler.mGpuMat.mShaderColor.Registers[i + 1];
  }
  for (int i = 0; i < 4; ++i) {
    tevKonstColors[i] = matHandler.mGpuMat.mShaderColor.Konstants[i];
  }

  // Indirect data
  TRY(librii::gpu::RunDisplayList(reader, matHandler, 64));
  for (u8 i = 0; i < numIndStages; ++i) {
    const auto& curScale =
        matHandler.mGpuMat.mIndirect.mIndTexScales[i > 1 ? i - 2 : i];
    scales[i] = librii::gx::IndirectTextureScalePair{
        static_cast<librii::gx::IndirectTextureScalePair::Selection>(
            curScale.ss0),
        static_cast<librii::gx::IndirectTextureScalePair::Selection>(
            curScale.ss1)};

    std::vector<std::string> _warnings;
    Result<librii::gx::IndirectMatrix> m =
        matHandler.mGpuMat.mIndirect.mIndMatrices[i].lift(_warnings);
    for (auto& warning : _warnings) {
      reader.warnAt(warning.c_str(), reader.tell() - 4, reader.tell());
    }
    indMatrices.push_back(TRY(m));
  }

  const std::array<u32, 9> texGenDlSizes{
      0,        // 0
      32,       // 1
      64,  64,  // 2, 3
      96,  96,  // 4, 5
      128, 128, // 6, 7
      160       // 8
  };
  EXPECT(numTexGens < 9);
  texGens.resize(numTexGens);
  TRY(librii::gpu::RunDisplayList(reader, matHandler,
                                  texGenDlSizes[numTexGens]));
  for (u8 i = 0; i < numTexGens; ++i) {
    texGens[i] = TRY(static_cast<Result<librii::gx::TexCoordGen>>(
        matHandler.mGpuMat.mTexture[i]));
  }

  return {};
}

Result<void> BinaryMaterial::read(oishii::BinaryReader& unsafeReader,
                                  gx::LowLevelGxMaterial* outShader) {
  rsl::SafeReader reader(unsafeReader);
  const auto start = reader.tell();

  TRY(reader.U32()); // size
  TRY(reader.S32()); // mdl offset
  name = TRY(reader.StringOfs(start));
  id = TRY(reader.U32());
  flag = TRY(reader.U32());

  // GenMode
  {
    genMode.read(reader);
    // TODO: Why?
    if (genMode.numChannels >= 2)
      genMode.numChannels = 2;
  }
  // Misc
  { //
    misc.read(reader);
  }

  const auto ofsTev = TRY(reader.S32());
  const auto nTex = TRY(reader.U32());
  EXPECT(nTex <= 8);
  const auto ofsSamplers = TRY(reader.S32());
  const auto ofsFur = TRY(reader.S32());
  const auto ofsUserData = TRY(reader.S32());
  const auto ofsDisplayLists = TRY(reader.S32());
  if (ofsFur || ofsUserData) {
    return std::unexpected(std::format(
        "Warning: Material {} uses Fur or UserData which is unsupported!",
        name));
  }

  for (auto& e : texPlttRuntimeObjs)
    e = TRY(reader.U8());

  TRY(texSrtData.read(reader));

  // ChanData
  {
    reader.seekSet(start + 0x3f0);
    TRY(chan.read(reader));
  }

  // TEV
  {
    auto tev_addr = start + ofsTev;
    tev.indirectStages.resize(genMode.numIndStages);
    tev.mStages.resize(genMode.numTevStages);
    TRY(librii::g3d::ReadTev(tev, unsafeReader, tev_addr));

    if (outShader != nullptr) {
      *outShader = tev;
    }
  }

  // Samplers
  {
    reader.seekSet(start + ofsSamplers);
    EXPECT(genMode.numTexGens == nTex);
    samplers.resize(nTex);
    for (auto& s : samplers)
      TRY(s.read(reader));
  }

  // Display Lists
  {
    reader.seekSet(start + ofsDisplayLists);
    TRY(dl.parse(unsafeReader, std::min<u32>(genMode.numIndStages, 4),
                 std::min<u32>(genMode.numTexGens, 8)));
  }

  return {};
}
// TODO: Reduce dependencies
Result<void> BinaryMaterial::writeBody(
    oishii::Writer& writer, u32 mat_start, NameTable& names,
    RelocWriter& linker,
    TextureSamplerMappingManager& tex_sampler_mappings) const {
  writeNameForward(names, writer, mat_start, name);
  writer.write<u32>(id);
  writer.write<u32>(flag);
  genMode.write(writer);
  misc.write(writer);
  linker.writeReloc<s32>("Mat" + std::to_string(mat_start),
                         "Shader" + std::to_string(tevId));
  writer.write<u32>(samplers.size());
  u32 sampler_offset = 0;
  if (samplers.size()) {
    sampler_offset = writePlaceholder(writer);
  } else {
    writer.write<s32>(0); // no samplers
  }
  writer.write<s32>(0); // fur
  writer.write<s32>(0); // ud
  const auto dl_offset = writePlaceholder(writer);
  for (auto& e : texPlttRuntimeObjs)
    writer.write<u8>(e);
  texSrtData.write(writer);
  chan.write(writer);

  // Not written if no samplers..
  if (sampler_offset != 0) {
    writeOffsetBackpatch(writer, sampler_offset, mat_start);
    for (int i = 0; i < samplers.size(); ++i) {
      const auto s_start = writer.tell();

      {
        const auto [entry_start, struct_start] =
            tex_sampler_mappings.from_mat(name, i);
        DebugReport("<material=\"%s\" sampler=%u>\n", name.c_str(), i);
        DebugReport("\tentry_start=%x, struct_start=%x\n",
                    (unsigned)entry_start, (unsigned)struct_start);
        oishii::Jump<oishii::Whence::Set, oishii::Writer> sg(writer,
                                                             entry_start);
        writer.write<s32>(mat_start - struct_start);
        writer.write<s32>(s_start - struct_start);
      }

      samplers[i].write(writer, names, s_start);
    }
  }
  writer.alignTo(32);
  writeOffsetBackpatch(writer, dl_offset, mat_start);
  TRY(dl.write(writer));
  return {};
}

Result<G3dMaterialData> fromBinMat(const BinaryMaterial& bin,
                                   const gx::LowLevelGxMaterial* smat) {
  G3dMaterialData mat;
  mat.name = bin.name;
  mat.id = bin.id;
  mat.flag = bin.flag;

  // GenMode
  {
    mat.texGens.resize(bin.genMode.numTexGens);
    mat.mStages.resize(bin.genMode.numTevStages);
    mat.indirectStages.resize(bin.genMode.numIndStages);
    mat.cullMode = bin.genMode.cullMode;
  }

  // Misc
  {
    mat.earlyZComparison = bin.misc.earlyZComparison;
    mat.lightSetIndex = bin.misc.lightSetIndex;
    mat.fogIndex = bin.misc.fogIndex;
    // Ignore reserved

    EXPECT(bin.misc.indMethod.size() >= bin.dl.indMatrices.size());
  }

  // TEV
  {
    if (smat != nullptr) {
      mat.mSwapTable = smat->mSwapTable;
      for (int i = 0; i < mat.indirectStages.size(); ++i) {
        mat.indirectStages[i].order.refMap =
            smat->indirectStages[i].order.refMap;
        mat.indirectStages[i].order.refCoord =
            smat->indirectStages[i].order.refCoord;
      }
      mat.mStages = smat->mStages;
    }
  }

  // Samplers
  {
    for (const auto& bs : bin.samplers) {
      librii::gx::GCMaterialData::SamplerData sampler = {
          .mTexture = bs.texture,
          .mPalette = bs.palette,
          // Ignore: RT ptrs
          // Ignore: texid/tlutid
          .mWrapU = bs.wrapU,
          .mWrapV = bs.wrapV,
          .bEdgeLod = bs.edgeLod,
          .bBiasClamp = bs.biasClamp,
          .mMaxAniso = bs.maxAniso,
          .mMinFilter = bs.minFilter,
          .mMagFilter = bs.magFilter,
          .mLodBias = bs.lodBias,
          // Ignore: reserved
      };
      mat.samplers.push_back(std::move(sampler));
    }
  }

  // DL
  {
    mat.alphaCompare = bin.dl.alphaCompare;
    mat.zMode = bin.dl.zMode;
    mat.blendMode = bin.dl.blendMode;
    mat.dstAlpha = bin.dl.dstAlpha;

    mat.tevColors[0] = {0xFF, 0xFF, 0xFF, 0xFF};
    for (int i = 0; i < 3; ++i)
      mat.tevColors[i + 1] = bin.dl.tevColors[i];
    mat.tevKonstColors = bin.dl.tevKonstColors;

    // TODO: Ensure safe
    mat.indirectStages.resize(4);
    for (int i = 0; i < bin.dl.scales.size(); ++i) {
      mat.indirectStages[i].scale = bin.dl.scales[i];
    }
    mat.indirectStages.resize(bin.genMode.numIndStages);
    mat.mIndMatrices.resize(bin.genMode.numIndStages); // TODO
    for (int i = 0; i < bin.dl.indMatrices.size(); ++i) {
      mat.mIndMatrices[i] = bin.dl.indMatrices[i];
      mat.mIndMatrices[i].method =
          TRY([](auto x) -> Result<gx::IndirectMatrix::Method> {
            switch (x) {
            case G3dIndMethod::Warp:
              return gx::IndirectMatrix::Method::Warp;
            case G3dIndMethod::NormalMap:
              return gx::IndirectMatrix::Method::NormalMap;
            case G3dIndMethod::SpecNormalMap:
              return gx::IndirectMatrix::Method::NormalMapSpec;
            case G3dIndMethod::Fur:
              return gx::IndirectMatrix::Method::Fur;
            case G3dIndMethod::Res0:
            case G3dIndMethod::Res1:
            case G3dIndMethod::User0:
            case G3dIndMethod::User1:
              EXPECT(false, "Unexpected indirect method");
              return gx::IndirectMatrix::Method::Warp;
            }
            EXPECT(false, "Invalid indirect method");
          }(bin.misc.indMethod[i]));
      mat.mIndMatrices[i].refLight = bin.misc.normalMapLightIndices[i];
    }

    for (int i = 0; i < mat.texGens.size(); ++i) {
      mat.texGens[i] = bin.dl.texGens[i];
    }
  }

  // TexSrtData
  {
    // TODO: Ignores flag
    const std::array<gx::GCMaterialData::CommonTransformModel, 3> cvtMtx{
        gx::GCMaterialData::CommonTransformModel::Maya,
        gx::GCMaterialData::CommonTransformModel::XSI,
        gx::GCMaterialData::CommonTransformModel::Max};
    const BinaryTexSrtData& sd = bin.texSrtData;
    const auto xfModel = cvtMtx[static_cast<u32>(sd.texMtxMode)];
    for (u8 i = 0; i < bin.samplers.size(); ++i) {
      gx::GCMaterialData::TexMatrix mtx;
      mtx.scale = sd.srt[i].scale;
      mtx.rotate = glm::radians(sd.srt[i].rotate_degrees);
      mtx.translate = sd.srt[i].translate;
      const BinaryTexMtxEffect& effect = sd.effect[i];
      mtx.camIdx = effect.camIdx;
      mtx.lightIdx = effect.lightIdx;
      mtx.transformModel = xfModel;
      mtx.option = gx::GCMaterialData::CommonMappingOption::NoSelection;
      // TODO: Are we necessarily loading the correct texgen?
      mtx.projection = bin.dl.texGens[i].func;

      switch (effect.mapMode) {
      default:
      case 0:
        mtx.method = gx::GCMaterialData::CommonMappingMethod::Standard;
        break;
      case 1:
        mtx.method =
            gx::GCMaterialData::CommonMappingMethod::EnvironmentMapping;
        break;
      case 2:
        mtx.method =
            gx::GCMaterialData::CommonMappingMethod::ViewProjectionMapping;
        break;
      case 3:
        mtx.method =
            gx::GCMaterialData::CommonMappingMethod::EnvironmentLightMapping;
        break;
      case 4:
        mtx.method =
            gx::GCMaterialData::CommonMappingMethod::EnvironmentSpecularMapping;
        break;
      // EGG
      case 5:
        mtx.method =
            gx::GCMaterialData::CommonMappingMethod::ManualProjectionMapping;
        break;
      }
      // Ignored: flag, effectMtx
      mat.texMatrices.push_back(mtx);
    }
  }

  // ChanData
  {
    bool seen_missing = false;
    size_t written_contiguously = 0;
    for (auto& chan : bin.chan.chan) {
      mat.chanData.push_back({chan.material, chan.ambient});
      if (chan.flag & BinaryChannelData::GDSetChanCtrl_COLOR0) {
        if (!seen_missing)
          ++written_contiguously;
        mat.colorChanControls.push_back(chan.xfCntrlColor);
      } else {
        seen_missing = true;
      }
      if (chan.flag & BinaryChannelData::GDSetChanCtrl_ALPHA0) {
        if (!seen_missing)
          ++written_contiguously;
        mat.colorChanControls.push_back(chan.xfCntrlAlpha);
      } else {
        seen_missing = true;
      }
    }
    // Very unlikely
    EXPECT(written_contiguously == mat.colorChanControls.size(),
           "Discontiguous channel configurations are unsupported");
  }
  return mat;
}

BinaryMaterial toBinMat(const G3dMaterialData& mat, u32 mat_idx) {
  BinaryMaterial bin;

  bin.name = mat.name;
  bin.id = mat_idx;
  bin.flag = (mat.flag & ~0x80000000) | (mat.xlu ? 0x80000000 : 0);

  // Since we assume contiguity
  const u8 numChannels =
      static_cast<u8>((mat.colorChanControls.size() + 1) / 2);

  bin.genMode = {.numTexGens = static_cast<u8>(mat.texGens.size()),
                 .numChannels = numChannels,
                 .numTevStages = static_cast<u8>(mat.mStages.size()),
                 .numIndStages = static_cast<u8>(mat.indirectStages.size()),
                 .cullMode = mat.cullMode};

  auto toIndMethod = [](const librii::gx::IndirectMatrix& mtx) {
    switch (mtx.method) {
    case gx::IndirectMatrix::Method::Warp:
    default: // TODO
      return G3dIndMethod::Warp;
    case gx::IndirectMatrix::Method::NormalMap:
      return G3dIndMethod::NormalMap;
    case gx::IndirectMatrix::Method::NormalMapSpec:
      return G3dIndMethod::SpecNormalMap;
    case gx::IndirectMatrix::Method::Fur:
      return G3dIndMethod::Fur;
    }
  };

  bin.misc = {
      .earlyZComparison = mat.earlyZComparison,
      .lightSetIndex = mat.lightSetIndex,
      .fogIndex = mat.fogIndex,
      .reserved = 0,
      .indMethod = mat.mIndMatrices | std::views::transform(toIndMethod) |
                   rsl::ToArray<4>(),
      .normalMapLightIndices =
          mat.mIndMatrices | rsl::ToArray<4>() |
          std::views::transform([](auto& x) { return x.refLight; }) |
          rsl::ToArray<4>(),
  };

  // TODO: Only copy fields needed
  bin.tev = mat;

  bin.samplers = rsl::enumerate(mat.samplers | rsl::ToList()) //
                 | std::views::transform([](const auto& x) {
                     const auto& [i, sampler] = x;
                     return BinarySampler{
                         .texture = sampler.mTexture,
                         .palette = sampler.mPalette,
                         .runtimeTexPtr = 0,
                         .runtimePlttPtr = 0,
                         .texMapId = static_cast<u32>(i),
                         .tlutId = static_cast<u32>(i),
                         .wrapU = sampler.mWrapU,
                         .wrapV = sampler.mWrapV,
                         .minFilter = sampler.mMinFilter,
                         .magFilter = sampler.mMagFilter,
                         .lodBias = sampler.mLodBias,
                         .maxAniso = sampler.mMaxAniso,
                         .biasClamp = sampler.bBiasClamp,
                         .edgeLod = sampler.bEdgeLod,
                         .reserved = 0,
                     };
                   }) //
                 | rsl::ToList();

  u32 tex_flags = 0;
  for (int i = mat.texMatrices.size() - 1; i >= 0; --i) {
    tex_flags = (tex_flags << 4) | BuildTexMatrixFlags(mat.texMatrices[i]);
  }

  // Unlike J3D, only one texmatrix mode per material
  using CommonTransformModel = librii::gx::GCMaterialData::CommonTransformModel;
  CommonTransformModel texmtx_mode = CommonTransformModel::Default;
  for (int i = 0; i < mat.texMatrices.size(); ++i) {
    texmtx_mode = mat.texMatrices[i].transformModel;
  }
  auto g3dTm = librii::g3d::TexMatrixMode::Maya;
  switch (texmtx_mode) {
  case librii::mtx::CommonTransformModel::Default:
  case librii::mtx::CommonTransformModel::Maya:
  default:
    g3dTm = librii::g3d::TexMatrixMode::Maya;
    break;
  case librii::mtx::CommonTransformModel::XSI:
    g3dTm = librii::g3d::TexMatrixMode::XSI;
    break;
  case librii::mtx::CommonTransformModel::Max:
    g3dTm = librii::g3d::TexMatrixMode::Max;
    break;
  }
  auto mapModeCvt = [](const auto& method) -> u8 {
    using CommonMappingMethod = librii::gx::GCMaterialData::CommonMappingMethod;
    switch (method) {
    case CommonMappingMethod::Standard:
    default:
      return 0;
    // In G3D, Light/Specular are used; in J3D, it'd be to the game
    case CommonMappingMethod::EnvironmentMapping:
    case CommonMappingMethod::ManualEnvironmentMapping:
      return 1;
    case CommonMappingMethod::ViewProjectionMapping:
      return 2;
    // EGG extension
    case CommonMappingMethod::ProjectionMapping:
      return 5;
    case CommonMappingMethod::EnvironmentLightMapping:
      return 3;
    case CommonMappingMethod::EnvironmentSpecularMapping:
      return 4;
    }
  };
  bin.texSrtData = {
      .flag = tex_flags,
      .texMtxMode = g3dTm,
      .srt = mat.texMatrices       //
             | std::views::take(8) //
             | std::views::transform(
                   [](const librii::gx::GCMaterialData::TexMatrix& mtx) {
                     return BinaryTexSrt{
                         .scale = mtx.scale,
                         .rotate_degrees = glm::degrees(mtx.rotate),
                         .translate = mtx.translate,
                     };
                   }) //
             | rsl::ToArray<8>(),
      .effect =
          mat.texMatrices       //
          | std::views::take(8) //
          | std::views::transform(
                [mapModeCvt](const librii::gx::GCMaterialData::TexMatrix& mtx) {
                  return BinaryTexMtxEffect{
                      .camIdx = mtx.camIdx,
                      .lightIdx = mtx.lightIdx,
                      .mapMode = mapModeCvt(mtx.method),
                      .flag = BinaryTexMtxEffect::EFFECT_MTX_IDENTITY,
                      .effectMtx = glm::mat3x4(1.0f),
                  };
                }) //
          | rsl::ToArray<8>(),
  };

  auto chanData = mat.chanData;
  auto colorChanControls = mat.colorChanControls;
  while (chanData.size() < 2) {
    // HACK: These are stored in uncleared memory by array-vector
    chanData.resize(chanData.size() + 1);
    if (colorChanControls.size() != (chanData.size() - 1) * 2) {
      continue;
    }
    colorChanControls.push_back(librii::gx::ChannelControl{
        .enabled = false,
        .Ambient = gx::ColorSource::Register,
        .Material = gx::ColorSource::Register,
        .lightMask = gx::LightID::None,
        .diffuseFn = gx::DiffuseFunction::None,
        .attenuationFn = gx::AttenuationFunction::Specular, // ID 0
    });
    colorChanControls.push_back(librii::gx::ChannelControl{
        .enabled = false,
        .Ambient = gx::ColorSource::Register,
        .Material = gx::ColorSource::Register,
        .lightMask = gx::LightID::None,
        .diffuseFn = gx::DiffuseFunction::None,
        .attenuationFn = gx::AttenuationFunction::Specular, // ID 0
    });
  }

  // TODO: Individually addressible channels
  //
  // Right now we generate contiguous COLOR0, ALPHA0, COLOR1, ALPHA1. Therefore
  // COLOR0 + COLOR1 + ALPHA1 would not be possible.
  //

  for (u8 i = 0; i < 2; ++i) {
    BinaryChannelData::Channel& chan = bin.chan.chan[i];
    chan.flag = BinaryChannelData::GDSetChanMatColor_COLOR0 |
                BinaryChannelData::GDSetChanMatColor_ALPHA0 |
                BinaryChannelData::GDSetChanAmbColor_COLOR0 |
                BinaryChannelData::GDSetChanAmbColor_ALPHA0;
    if (i * 2 < mat.colorChanControls.size()) {
      chan.flag |= BinaryChannelData::GDSetChanCtrl_COLOR0;
    }
    if (i * 2 + 1 < mat.colorChanControls.size()) {
      chan.flag |= BinaryChannelData::GDSetChanCtrl_ALPHA0;
    }

    chan.material = chanData[i].matColor;
    chan.ambient = chanData[i].ambColor;

    // It's impossible to get the hex 0 we see by calling this function, as
    // attnEnable is enabled when attenuationFn != None, and attnSelect when
    // attenuationFn != Specular.
    if (i * 2 < mat.colorChanControls.size()) {
      chan.xfCntrlColor.from(colorChanControls[i * 2]);
    }
    if (i * 2 + 1 < mat.colorChanControls.size()) {
      chan.xfCntrlAlpha.from(colorChanControls[i * 2 + 1]);
    }
  }

  bin.dl.alphaCompare = mat.alphaCompare;
  bin.dl.zMode = mat.zMode;
  bin.dl.blendMode = mat.blendMode;
  bin.dl.dstAlpha = mat.dstAlpha;

  bin.dl.tevColors = mat.tevColors | std::views::drop(1) | rsl::ToArray<3>();
  bin.dl.tevKonstColors = mat.tevKonstColors;

  for (int i = 0; i < mat.indirectStages.size(); ++i)
    bin.dl.scales[i] = mat.indirectStages[i].scale;
  bin.dl.indMatrices = mat.mIndMatrices;

  bin.dl.texGens = mat.texGens;
  return bin;
}

bool readMaterial(G3dMaterialData& mat, oishii::BinaryReader& reader,
                  bool ignore_tev) {
  BinaryMaterial bin;
  gx::LowLevelGxMaterial smat;
  if (!bin.read(reader, ignore_tev ? nullptr : &smat)) {
    return false;
  }
  auto ok = fromBinMat(bin, ignore_tev ? nullptr : &smat);
  if (!ok.has_value()) {
    return false;
  }
  mat = *ok;
  return true;
}

void WriteMaterialBody(size_t mat_start, oishii::Writer& writer,
                       NameTable& names, const G3dMaterialData& mat,
                       u32 mat_idx, RelocWriter& linker,
                       TextureSamplerMappingManager& tex_sampler_mappings) {
  auto bin = toBinMat(mat, mat_idx);
  bin.writeBody(writer, mat_start, names, linker, tex_sampler_mappings);
}

} // namespace librii::g3d
