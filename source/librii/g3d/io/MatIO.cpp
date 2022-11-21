#include "MatIO.hpp"
#include <librii/gpu/DLBuilder.hpp>
#include <librii/gpu/DLPixShader.hpp>

namespace librii::g3d {

inline void operator<<(librii::gx::Color& out, oishii::BinaryReader& reader) {
  out = librii::gx::readColorComponents(
      reader, librii::gx::VertexBufferType::Color::rgba8);
}

inline void operator>>(const librii::gx::Color& out, oishii::Writer& writer) {
  librii::gx::writeColorComponents(writer, out,
                                   librii::gx::VertexBufferType::Color::rgba8);
}

void WriteCommonTransformModel(oishii::Writer& writer,
                               librii::mtx::CommonTransformModel mdl) {
  switch (mdl) {
  case librii::mtx::CommonTransformModel::Default:
  case librii::mtx::CommonTransformModel::Maya:
  default:
    writer.write<u32>(0);
    break;
  case librii::mtx::CommonTransformModel::XSI:
    writer.write<u32>(1);
    break;
  case librii::mtx::CommonTransformModel::Max:
    writer.write<u32>(2);
    break;
  }
}

void WriteTexMatrix(oishii::Writer& writer,
                    const librii::gx::GCMaterialData::TexMatrix& mtx) {
  writer.write<f32>(mtx.scale.x);
  writer.write<f32>(mtx.scale.y);
  writer.write<f32>(glm::degrees(mtx.rotate));
  writer.write<f32>(mtx.translate.x);
  writer.write<f32>(mtx.translate.y);
}
void WriteTexMatrixIdentity(oishii::Writer& writer) {
  writer.write<f32>(1.0f);
  writer.write<f32>(1.0f);
  writer.write<f32>(0.0f);
  writer.write<f32>(0.0f);
  writer.write<f32>(0.0f);
}
void WriteCommonMappingMethod(
    librii::gx::GCMaterialData::CommonMappingMethod method,
    oishii::Writer& writer) {
  using CommonMappingMethod = librii::gx::GCMaterialData::CommonMappingMethod;
  switch (method) {
  case CommonMappingMethod::Standard:
  default:
    writer.write<u8>(0);
    break;
  case CommonMappingMethod::EnvironmentMapping:
  case CommonMappingMethod::ManualEnvironmentMapping: // In G3D, Light/Specular
                                                      // are used; in J3D, it'd
                                                      // be to the game
    writer.write<u8>(1);
    break;
  case CommonMappingMethod::ViewProjectionMapping:
    writer.write<u8>(2);
    break;
    // EGG extension
  case CommonMappingMethod::ProjectionMapping:
    writer.write<u8>(5);
    break;
  case CommonMappingMethod::EnvironmentLightMapping:
    writer.write<u8>(3);
    break;
  case CommonMappingMethod::EnvironmentSpecularMapping:
    writer.write<u8>(4);
    break;
  }
}
void WriteTexMatrixEffect(oishii::Writer& writer,
                          const librii::gx::GCMaterialData::TexMatrix& mtx,
                          std::array<f32, 12>& ident34) {
  // printf("CamIdx: %i, LIdx: %i\n", (signed)mtx.camIdx,
  //        (signed)mtx.lightIdx);
  writer.write<s8>((s8)mtx.camIdx);
  writer.write<s8>((s8)mtx.lightIdx);

  WriteCommonMappingMethod(mtx.method, writer);
  // No effect matrix support yet
  writer.write<u8>(1);

  for (auto d : ident34)
    writer.write<f32>(d);
}
void WriteTexMatrixEffectDefault(oishii::Writer& writer,
                                 std::array<f32, 12>& ident34) {
  writer.write<u8>(0xff); // cam
  writer.write<u8>(0xff); // light
  writer.write<u8>(0);    // map
  writer.write<u8>(1);    // flag

  for (auto d : ident34)
    writer.write<f32>(d);
}
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
void WriteMaterialMisc(oishii::Writer& writer, const G3dMaterialData& mat) {
  writer.write<u8>(mat.earlyZComparison);
  writer.write<u8>(mat.lightSetIndex);
  writer.write<u8>(mat.fogIndex);
  writer.write<u8>(0); // pad

  // TODO: Properly set this in the UI
  //
  auto indConfig = mat.indConfig;
  if (mat.indirectStages.size() > indConfig.size()) {
    DebugReport("[NOTE] mat.indirectStages.size() > indConfig.size(), will be "
                "corrected on save.\n");
    indConfig.resize(mat.indirectStages.size());
  }

  assert(mat.indirectStages.size() <= indConfig.size());
  for (u8 i = 0; i < mat.indirectStages.size(); ++i)
    writer.write<u8>(static_cast<u8>(indConfig[i].method));
  for (u8 i = mat.indirectStages.size(); i < 4; ++i)
    writer.write<u8>(0);
  for (u8 i = 0; i < mat.indirectStages.size(); ++i)
    writer.write<u8>(indConfig[i].normalMapLightRef);
  for (u8 i = mat.indirectStages.size(); i < 4; ++i)
    writer.write<u8>(0xff);
}
void WriteMaterialGenMode(oishii::Writer& writer, const G3dMaterialData& mat) {
  writer.write<u8>(static_cast<u8>(mat.texGens.size()));
  writer.write<u8>(static_cast<u8>(mat.chanData.size()));
  writer.write<u8>(static_cast<u8>(mat.mStages.size()));
  writer.write<u8>(static_cast<u8>(mat.indirectStages.size()));
  writer.write<u32>(static_cast<u32>(mat.cullMode));
}

static void writeMaterialDisplayList(const gx::GCMaterialData& mat,
                                     oishii::Writer& writer) {
  MAYBE_UNUSED const auto dl_start = writer.tell();
  DebugReport("Mat dl start: %x\n", (unsigned)writer.tell());
  librii::gpu::DLBuilder dl(writer);
  {
    dl.setAlphaCompare(mat.alphaCompare);
    dl.setZMode(mat.zMode);
    dl.setBlendMode(mat.blendMode);
    dl.setDstAlpha(false, 0); // TODO
    dl.align();
  }
  assert(writer.tell() - dl_start == 0x20);
  {
    for (int i = 0; i < 3; ++i)
      dl.setTevColor(i + 1, mat.tevColors[i + 1]);
    dl.align(); // pad 4
    for (int i = 0; i < 4; ++i)
      dl.setTevKColor(i, mat.tevKonstColors[i]);
    dl.align(); // 24
  }
  assert(writer.tell() - dl_start == 0xa0);
  {
    std::array<librii::gx::IndirectTextureScalePair, 4> scales;
    for (int i = 0; i < mat.indirectStages.size(); ++i)
      scales[i] = mat.indirectStages[i].scale;
    dl.setIndTexCoordScale(0, scales[0], scales[1]);
    dl.setIndTexCoordScale(2, scales[2], scales[3]);

    const auto write_ind_mtx = [&](std::size_t i) {
      if (mat.mIndMatrices.size() > i)
        dl.setIndTexMtx(i, mat.mIndMatrices[i]);
      else
        writer.skip(5 * 3);
    };
    write_ind_mtx(0);
    dl.align(); // 7
    write_ind_mtx(1);
    write_ind_mtx(2);
    dl.align();
  }
  assert(writer.tell() - dl_start == 0xe0);
  {
    for (int i = 0; i < mat.texGens.size(); ++i)
      dl.setTexCoordGen(i, mat.texGens[i]);
    writer.skip(18 * (8 - mat.texGens.size()));
    dl.align();
  }
  assert(writer.tell() - dl_start == 0x180);
}

bool readMaterial(G3dMaterialData& mat, oishii::BinaryReader& reader, bool ignore_tev) {
  const auto start = reader.tell();

  reader.read<u32>(); // size
  reader.read<s32>(); // mdl offset
  mat.name = readName(reader, start);
  mat.id = reader.read<u32>();
  mat.flag = reader.read<u32>();

  // Gen info
  mat.texGens.resize(reader.read<u8>());
  auto nColorChan = reader.read<u8>();
  if (nColorChan >= 2)
    nColorChan = 2;
  mat.mStages.resize(reader.read<u8>());
  mat.indirectStages.resize(reader.read<u8>());
  mat.cullMode = static_cast<librii::gx::CullMode>(reader.read<u32>());

  // Misc
  mat.earlyZComparison = reader.read<u8>();
  mat.lightSetIndex = reader.read<s8>();
  mat.fogIndex = reader.read<s8>();
  reader.read<u8>();
  const auto indSt = reader.tell();
  for (u8 i = 0; i < mat.indirectStages.size(); ++i) {
    librii::g3d::G3dIndConfig cfg;
    cfg.method = static_cast<librii::g3d::G3dIndMethod>(reader.read<u8>());
    cfg.normalMapLightRef = reader.peekAt<s8>(4);
    mat.indConfig.push_back(cfg);
  }
  reader.seekSet(indSt + 4 + 4);

  const auto ofsTev = reader.read<s32>();
  const auto nTex = reader.read<u32>();
  if (nTex > 8) {
    return false;
  }
  const auto [ofsSamplers, ofsFur, ofsUserData, ofsDisplayLists] =
      reader.readX<s32, 4>();
  if (ofsFur || ofsUserData)
    printf("Warning: Material %s uses Fur or UserData which is unsupported!\n",
           mat.name.c_str());

  using GCMaterialData = librii::gx::GCMaterialData;

  // Texture and palette objects are set on runtime.
  reader.skip((4 + 8 * 12) + (4 + 8 * 32));

  // Texture transformations
  reader.read<u32>(); // skip flag, TODO: verify
  const u32 mtxMode = reader.read<u32>();
  const std::array<GCMaterialData::CommonTransformModel, 3> cvtMtx{
      GCMaterialData::CommonTransformModel::Maya,
      GCMaterialData::CommonTransformModel::XSI,
      GCMaterialData::CommonTransformModel::Max};
  const auto xfModel = cvtMtx[mtxMode];

  for (u8 i = 0; i < nTex; ++i) {
    GCMaterialData::TexMatrix mtx;
    mtx.scale.x = reader.read<f32>();
    mtx.scale.y = reader.read<f32>();
    mtx.rotate = glm::radians(reader.read<f32>());
    mtx.translate.x = reader.read<f32>();
    mtx.translate.y = reader.read<f32>();
    mat.texMatrices.push_back(mtx);
  }
  reader.seekSet(start + 0x250);
  for (u8 i = 0; i < nTex; ++i) {
    auto* mtx = &mat.texMatrices[i];

    mtx->camIdx = reader.read<s8>();
    mtx->lightIdx = reader.read<s8>();
    // printf("[read] CamIDX: %i, LIDX:%i\n", (signed)mtx->camIdx,
    //        (signed)mtx->lightIdx);
    const u8 mapMode = reader.read<u8>();

    mtx->transformModel = xfModel;
    mtx->option = GCMaterialData::CommonMappingOption::NoSelection;
    // Projection needs to be copied from texgen

    switch (mapMode) {
    default:
    case 0:
      mtx->method = GCMaterialData::CommonMappingMethod::Standard;
      break;
    case 1:
      mtx->method = GCMaterialData::CommonMappingMethod::EnvironmentMapping;
      break;
    case 2:
      mtx->method = GCMaterialData::CommonMappingMethod::ViewProjectionMapping;
      break;
    case 3:
      mtx->method =
          GCMaterialData::CommonMappingMethod::EnvironmentLightMapping;
      break;
    case 4:
      mtx->method =
          GCMaterialData::CommonMappingMethod::EnvironmentSpecularMapping;
      break;
    // EGG
    case 5:
      mtx->method =
          GCMaterialData::CommonMappingMethod::ManualProjectionMapping;
      break;
    }

    reader.read<u8>(); // effect mtx flag
    // G3D effectmatrix is 3x4. J3D is 4x4
    reader.skip(3 * 4 * sizeof(f32));
    // for (auto& f : mtx->effectMatrix)
    //   f = reader.read<f32>();
  }
  // reader.seek((8 - nTex)* (4 + (4 * 4 * sizeof(f32))));
  reader.seekSet(start + 0x3f0);
  mat.chanData.resize(0);
  for (u8 i = 0; i < nColorChan; ++i) {
    // skip runtime flag
    const auto flag = reader.read<u32>();
    (void)flag;
    librii::gx::Color matClr, ambClr;
    matClr.r = reader.read<u8>();
    matClr.g = reader.read<u8>();
    matClr.b = reader.read<u8>();
    matClr.a = reader.read<u8>();

    ambClr.r = reader.read<u8>();
    ambClr.g = reader.read<u8>();
    ambClr.b = reader.read<u8>();
    ambClr.a = reader.read<u8>();

    mat.chanData.push_back({matClr, ambClr});
    librii::gpu::LitChannel tmp;
    // Color
    tmp.hex = reader.read<u32>();
    mat.colorChanControls.push_back(tmp);
    // Alpha
    tmp.hex = reader.read<u32>();
    mat.colorChanControls.push_back(tmp);
  }

  // TEV
  if (!ignore_tev) {
    auto tev_addr = start + ofsTev;
    librii::g3d::ReadTev(mat, reader, tev_addr);
  }

  // Samplers
  reader.seekSet(start + ofsSamplers);
  if (mat.texGens.size() != nTex) {
    return false;
  }
  for (u8 i = 0; i < nTex; ++i) {
    GCMaterialData::SamplerData sampler;

    const auto sampStart = reader.tell();
    sampler.mTexture = readName(reader, sampStart);
    sampler.mPalette = readName(reader, sampStart);
    reader.skip(8);     // skip runtime pointers
    reader.read<u32>(); // skip tex id for now
    reader.read<u32>(); // skip tlut id for now
    sampler.mWrapU =
        static_cast<librii::gx::TextureWrapMode>(reader.read<u32>());
    sampler.mWrapV =
        static_cast<librii::gx::TextureWrapMode>(reader.read<u32>());
    sampler.mMinFilter =
        static_cast<librii::gx::TextureFilter>(reader.read<u32>());
    sampler.mMagFilter =
        static_cast<librii::gx::TextureFilter>(reader.read<u32>());
    sampler.mLodBias = reader.read<f32>();
    sampler.mMaxAniso =
        static_cast<librii::gx::AnisotropyLevel>(reader.read<u32>());
    sampler.bBiasClamp = reader.read<u8>();
    sampler.bEdgeLod = reader.read<u8>();
    reader.skip(2);
    mat.samplers.push_back(std::move(sampler));
  }

  // Display Lists
  reader.seekSet(start + ofsDisplayLists);
  {
    librii::gpu::QDisplayListMaterialHandler matHandler(mat);

    // Pixel data
    librii::gpu::RunDisplayList(reader, matHandler, 32);
    mat.alphaCompare = matHandler.mGpuMat.mPixel.mAlphaCompare;
    mat.zMode = matHandler.mGpuMat.mPixel.mZMode;
    mat.blendMode = matHandler.mGpuMat.mPixel.mBlendMode;
    // TODO: Dst alpha

    // Uniform data
    librii::gpu::RunDisplayList(reader, matHandler, 128);
    mat.tevColors[0] = {0xff, 0xff, 0xff, 0xff};
    for (int i = 0; i < 3; ++i) {
      mat.tevColors[i + 1] = matHandler.mGpuMat.mShaderColor.Registers[i + 1];
    }
    for (int i = 0; i < 4; ++i) {
      mat.tevKonstColors[i] = matHandler.mGpuMat.mShaderColor.Konstants[i];
    }

    // Indirect data
    librii::gpu::RunDisplayList(reader, matHandler, 64);
    for (u8 i = 0; i < mat.indirectStages.size(); ++i) {
      const auto& curScale =
          matHandler.mGpuMat.mIndirect.mIndTexScales[i > 1 ? i - 2 : i];
      mat.indirectStages[i].scale = librii::gx::IndirectTextureScalePair{
          static_cast<librii::gx::IndirectTextureScalePair::Selection>(
              curScale.ss0),
          static_cast<librii::gx::IndirectTextureScalePair::Selection>(
              curScale.ss1)};

      mat.mIndMatrices.push_back(matHandler.mGpuMat.mIndirect.mIndMatrices[i]);
    }

    const std::array<u32, 9> texGenDlSizes{
        0,        // 0
        32,       // 1
        64,  64,  // 2, 3
        96,  96,  // 4, 5
        128, 128, // 6, 7
        160       // 8
    };
    librii::gpu::RunDisplayList(reader, matHandler,
                                texGenDlSizes[mat.texGens.size()]);
    for (u8 i = 0; i < mat.texGens.size(); ++i) {
      mat.texGens[i] = matHandler.mGpuMat.mTexture[i];
      mat.texMatrices[i].projection = mat.texGens[i].func;
    }
  }

  return true;
}

void WriteMaterial(size_t mat_start, oishii::Writer& writer,
                   NameTable& names, const G3dMaterialData& mat, u32 mat_idx,
                   RelocWriter& linker, const ShaderAllocator& shader_allocator,
                   TextureSamplerMappingManager& tex_sampler_mappings) {
  DebugReport("MAT_START %x\n", (u32)mat_start);
  DebugReport("MAT_NAME %x\n", writer.tell());
  writeNameForward(names, writer, mat_start, mat.name);
  DebugReport("MATIDAT %x\n", writer.tell());
  writer.write<u32>(mat_idx);
  u32 flag = mat.flag;
  flag = (flag & ~0x80000000) | (mat.xlu ? 0x80000000 : 0);
  writer.write<u32>(flag);
  WriteMaterialGenMode(writer, mat);
  WriteMaterialMisc(writer, mat);
  linker.writeReloc<s32>("Mat" + std::to_string(mat_start),
                         shader_allocator.getShaderIDName(mat));
  writer.write<u32>(mat.samplers.size());
  u32 sampler_offset = 0;
  if (mat.samplers.size()) {
    sampler_offset = writePlaceholder(writer);
  } else {
    writer.write<s32>(0); // no samplers
    printf(">> Mat %s has no samplers.\n", mat.name.c_str());
  }
  writer.write<s32>(0); // fur
  writer.write<s32>(0); // ud
  const auto dl_offset = writePlaceholder(writer);

  // Texture and palette objects are set on runtime.
  writer.skip((4 + 8 * 12) + (4 + 8 * 32));

  // Texture transformations

  u32 tex_flags = 0;
  for (int i = mat.texMatrices.size() - 1; i >= 0; --i) {
    tex_flags = (tex_flags << 4) | BuildTexMatrixFlags(mat.texMatrices[i]);
  }
  writer.write<u32>(tex_flags);
  // Unlike J3D, only one texmatrix mode per material
  using CommonTransformModel = librii::gx::GCMaterialData::CommonTransformModel;
  CommonTransformModel texmtx_mode = CommonTransformModel::Default;
  for (int i = 0; i < mat.texMatrices.size(); ++i) {
    texmtx_mode = mat.texMatrices[i].transformModel;
  }

  WriteCommonTransformModel(writer, texmtx_mode);
  for (int i = 0; i < mat.texMatrices.size(); ++i) {
    auto& mtx = mat.texMatrices[i];

    WriteTexMatrix(writer, mtx);
  }
  for (int i = mat.texMatrices.size(); i < 8; ++i) {
    WriteTexMatrixIdentity(writer);
  }
  std::array<f32, 3 * 4> ident34{1.0, 0.0, 0.0, 0.0, 0.0, 1.0,
                                 0.0, 0.0, 0.0, 0.0, 1.0, 0.0};
  for (int i = 0; i < mat.texMatrices.size(); ++i) {
    auto& mtx = mat.texMatrices[i];

    WriteTexMatrixEffect(writer, mtx, ident34);
  }
  for (int i = mat.texMatrices.size(); i < 8; ++i) {
    WriteTexMatrixEffectDefault(writer, ident34);
  }

  for (u8 i = 0; i < mat.chanData.size(); ++i) {
    // TODO: flag
    writer.write<u32>(0x3f);
    mat.chanData[i].matColor >> writer;
    mat.chanData[i].ambColor >> writer;

    librii::gpu::LitChannel clr, alpha;

    clr.from(mat.colorChanControls[i]);
    alpha.from(mat.colorChanControls[i + 1]);

    writer.write<u32>(clr.hex);
    writer.write<u32>(alpha.hex);
  }
  for (u8 i = mat.chanData.size(); i < 2; ++i) {
    writer.write<u32>(0x3f); // TODO: flag, reset on runtime
    writer.write<u32>(0);    // mclr
    writer.write<u32>(0);    // aclr
    writer.write<u32>(0);    // clr
    writer.write<u32>(0);    // alph
  }

  // Not written if no samplers..
  if (sampler_offset != 0) {
    writeOffsetBackpatch(writer, sampler_offset, mat_start);
    for (int i = 0; i < mat.samplers.size(); ++i) {
      const auto s_start = writer.tell();
      const auto& sampler = mat.samplers[i];

      {
        const auto [entry_start, struct_start] =
            tex_sampler_mappings.from_mat(&mat, i);
        DebugReport("<material=\"%s\" sampler=%u>\n", mat.name.c_str(), i);
        DebugReport("\tentry_start=%x, struct_start=%x\n",
                    (unsigned)entry_start, (unsigned)struct_start);
        oishii::Jump<oishii::Whence::Set, oishii::Writer> sg(writer,
                                                             entry_start);
        writer.write<s32>(mat_start - struct_start);
        writer.write<s32>(s_start - struct_start);
      }

      writeNameForward(names, writer, s_start, sampler.mTexture);
      writeNameForward(names, writer, s_start, sampler.mPalette);
      writer.skip(8);       // runtime pointers
      writer.write<u32>(i); // gpu texture slot
      writer.write<u32>(i); // no palette support
      writer.write<u32>(static_cast<u32>(sampler.mWrapU));
      writer.write<u32>(static_cast<u32>(sampler.mWrapV));
      writer.write<u32>(static_cast<u32>(sampler.mMinFilter));
      writer.write<u32>(static_cast<u32>(sampler.mMagFilter));
      writer.write<f32>(sampler.mLodBias);
      writer.write<u32>(static_cast<u32>(sampler.mMaxAniso));
      writer.write<u8>(sampler.bBiasClamp);
      writer.write<u8>(sampler.bEdgeLod);
      writer.skip(2);
    }
  }
  writer.alignTo(32);
  writeOffsetBackpatch(writer, dl_offset, mat_start);
  writeMaterialDisplayList(mat, writer);
}

} // namespace librii::g3d
