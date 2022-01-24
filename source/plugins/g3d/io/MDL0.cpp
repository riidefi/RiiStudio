#include "Common.hpp"
#include <core/common.h>
#include <librii/g3d/io/DictWriteIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <librii/gpu/DLBuilder.hpp>
#include <librii/gpu/DLPixShader.hpp>
#include <librii/gpu/GPUMaterial.hpp>
#include <librii/gx.h>
#include <plugins/g3d/collection.hpp>
#include <plugins/g3d/util/NameTable.hpp>

namespace riistudio::g3d {

struct ShaderAllocator {
  void alloc(const librii::g3d::G3dShader& shader) {
    auto found = std::find(shaders.begin(), shaders.end(), shader);
    if (found == shaders.end()) {
      matToShaderMap.emplace_back(shaders.size());
      shaders.emplace_back(shader);
    } else {
      matToShaderMap.emplace_back(found - shaders.begin());
    }
  }

  int find(const librii::g3d::G3dShader& shader) const {
    if (const auto found = std::find(shaders.begin(), shaders.end(), shader);
        found != shaders.end()) {
      return std::distance(shaders.begin(), found);
    }

    return -1;
  }

  auto size() const { return shaders.size(); }

  std::string getShaderIDName(const librii::g3d::G3dShader& shader) const {
    const int found = find(shader);
    assert(found >= 0);
    return "Shader" + std::to_string(found);
  }

  std::vector<librii::g3d::G3dShader> shaders;
  std::vector<u32> matToShaderMap;
};

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

struct RenderList {
  struct DrawCmd {
    u16 matIdx;
    u16 polyIdx;
    u16 boneIdx;
    u8 prio;

    RenderCommand type = RenderCommand::Draw;

    void write(oishii::Writer& writer) const {
      writer.writeUnaligned<u8>(static_cast<u8>(type));
      if (type == RenderCommand::Draw) {
        writer.writeUnaligned<u16>(matIdx);
        writer.writeUnaligned<u16>(polyIdx);
        writer.writeUnaligned<u16>(boneIdx);
        writer.writeUnaligned<u8>(prio);
      } else if (type == RenderCommand::NodeDescendence) {
        // Hack: Only for single-bone setups
        writer.writeUnaligned<u32>(0); // 0 : 0
      }
    }
  };
  std::vector<DrawCmd> cmds;
  std::string name;
  std::string getName() const { return name; }
};
// Does not write size or mdl0 offset
template <typename T, bool HasMinimum, bool HasDivisor,
          librii::gx::VertexBufferKind kind>
void writeGenericBuffer(
    const GenericBuffer<T, HasMinimum, HasDivisor, kind>& buf,
    oishii::Writer& writer, u32 header_start, NameTable& names) {
  const auto backpatch_array_ofs = writePlaceholder(writer);
  writeNameForward(names, writer, header_start, buf.getName());
  writer.write<u32>(buf.mId);
  writer.write<u32>(static_cast<u32>(buf.mQuantize.mComp.position));
  writer.write<u32>(static_cast<u32>(buf.mQuantize.mType.generic));
  if constexpr (HasDivisor) {
    writer.write<u8>(buf.mQuantize.divisor);
    writer.write<u8>(buf.mQuantize.stride);
  } else {
    writer.write<u8>(buf.mQuantize.stride);
    writer.write<u8>(0);
  }
  writer.write<u16>(buf.mEntries.size());
  // TODO: min/max
  if constexpr (HasMinimum) {
    T min;
    T max;
    for (int c = 0; c < T::length(); ++c) {
      min[c] = +1'000'000'000.0f;
      max[c] = -1'000'000'000.0f;
    }
    for (auto& elem : buf.mEntries) {
      for (int c = 0; c < elem.length(); ++c) {
        min[c] = std::min(elem[c], min[c]);
        max[c] = std::max(elem[c], max[c]);
      }
    }

    min >> writer;
    max >> writer;
  }

  writer.alignTo(32);
  writeOffsetBackpatch(writer, backpatch_array_ofs, header_start);

  const auto nComponents =
      librii::gx::computeComponentCount(kind, buf.mQuantize.mComp);

  for (auto& entry : buf.mEntries) {
    librii::gx::writeComponents(writer, entry, buf.mQuantize.mType, nComponents,
                                buf.mQuantize.divisor);
  }
  writer.alignTo(32);
} // namespace riistudio::g3d

static void writeMaterialDisplayList(const libcube::GCMaterialData& mat,
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

void writeVertexDataDL(const libcube::IndexedPolygon& poly,
                       const MatrixPrimitive& mp, oishii::Writer& writer) {
  using VATAttrib = librii::gx::VertexAttribute;
  using VATType = librii::gx::VertexAttributeType;

  for (auto& prim : mp.mPrimitives) {
    writer.write<u8>(librii::gx::EncodeDrawPrimitiveCommand(prim.mType));
    writer.write<u16>(prim.mVertices.size());
    for (const auto& v : prim.mVertices) {
      for (int a = 0; a < (int)VATAttrib::Max; ++a) {
        if (poly.getVcd()[(VATAttrib)a]) {
          switch (poly.getVcd().mAttributes.at((VATAttrib)a)) {
          case VATType::None:
            break;
          case VATType::Byte:
            writer.write<u8>(v[(VATAttrib)a]);
            break;
          case VATType::Short:
            writer.write<u16>(v[(VATAttrib)a]);
            break;
          case VATType::Direct:
            if (((VATAttrib)a) != VATAttrib::PositionNormalMatrixIndex) {
              assert(!"Direct vertex data is unsupported.");
              abort();
            }
            writer.write<u8>(v[(VATAttrib)a]);
            break;
          default:
            assert("!Unknown vertex attribute format.");
            abort();
          }
        }
      }
    }
  }

  // DL pad
  while (writer.tell() % 32)
    writer.write<u8>(0);
}
void WriteTexMatrix(oishii::Writer& writer,
                    const libcube::GCMaterialData::TexMatrix& mtx) {
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
    libcube::GCMaterialData::CommonMappingMethod method,
    oishii::Writer& writer) {
  using CommonMappingMethod = libcube::GCMaterialData::CommonMappingMethod;
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
                          const libcube::GCMaterialData::TexMatrix& mtx,
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
u32 BuildTexMatrixFlags(const libcube::GCMaterialData::TexMatrix& mtx) {
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
void WriteMaterialMisc(oishii::Writer& writer,
                       const riistudio::g3d::Material& mat) {
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
void WriteMaterialGenMode(oishii::Writer& writer,
                          const riistudio::g3d::Material& mat) {
  writer.write<u8>(static_cast<u8>(mat.texGens.size()));
  writer.write<u8>(static_cast<u8>(mat.chanData.size()));
  writer.write<u8>(static_cast<u8>(mat.mStages.size()));
  writer.write<u8>(static_cast<u8>(mat.indirectStages.size()));
  writer.write<u32>(static_cast<u32>(mat.cullMode));
}
void WriteBone(riistudio::g3d::NameTable& names, oishii::Writer& writer,
               const size_t& bone_start, const riistudio::g3d::Bone& bone,
               u32& bone_id) {
  writeNameForward(names, writer, bone_start, bone.getName());
  writer.write<u32>(bone_id++);
  writer.write<u32>(bone.matrixId);
  writer.write<u32>(computeFlag(bone));
  writer.write<u32>(bone.billboardType);
  writer.write<u32>(0); // TODO: ref
  bone.mScaling >> writer;
  bone.mRotation >> writer;
  bone.mTranslation >> writer;
  bone.mVolume.min >> writer;
  bone.mVolume.max >> writer;

  // Parent, Child, Left, Right
  writer.write<s32>(0);
  writer.write<s32>(0);
  writer.write<s32>(0);
  writer.write<s32>(0);

  writer.write<u32>(0); // user data

  // Recomputed on runtime?
  // Mtx + Inverse, 3x4 f32
  std::array<f32, 2 * 3 * 4> matrix_data{
      1.0, 0.0,  0.0, 0.0, 0.0,  1.0, 0.0,  0.0, 0.0, 0.0,  1.0, 0.0,
      1.0, -0.0, 0.0, 0.0, -0.0, 1.0, -0.0, 0.0, 0.0, -0.0, 1.0, 0.0};
  for (auto d : matrix_data)
    writer.write<f32>(d);
}
void WriteRenderList(const riistudio::g3d::RenderList& list,
                     oishii::Writer& writer) {
  for (auto& cmd : list.cmds)
    cmd.write(writer);
  writer.writeUnaligned<u8>(1); // terminator
}

void WriteShader(RelocWriter& linker, oishii::Writer& writer,
                 const librii::g3d::G3dShader& shader, std::size_t shader_start,
                 int shader_id) {
  DebugReport("Shader at %x\n", (unsigned)shader_start);
  linker.label("Shader" + std::to_string(shader_id), shader_start);

  librii::g3d::WriteTevBody(writer, shader_id, shader);
}

struct TextureSamplerMapping {
  TextureSamplerMapping() = default;
  TextureSamplerMapping(std::string n, u32 pos) : name(n) {
    entries.emplace_back(pos);
  }

  const std::string& getName() const { return name; }
  std::string name;
  llvm::SmallVector<u32, 1> entries; // stream position
};
struct TextureSamplerMappingManager {
  void add_entry(const std::string& name, const Material* mat,
                 int mat_sampl_index) {
    if (auto found = std::find_if(entries.begin(), entries.end(),
                                  [name](auto& e) { return e.name == name; });
        found != entries.end()) {
      matMap[{mat, mat_sampl_index}] = {found - entries.begin(),
                                        found->entries.size()};
      found->entries.emplace_back(0);
      return;
    }
    matMap[{mat, mat_sampl_index}] = {entries.size(), 0};
    entries.emplace_back(name, 0);
  }
  std::pair<u32, u32> from_mat(const Material* mat, int sampl_idx) {
    std::pair key{mat, sampl_idx};
    if (!matMap.contains(key)) {
      // TODO: Invalid tex ref.
      return {0, 0};
    }
    assert(matMap.contains(key));
    const auto [entry_n, sub_n] = matMap.at(key);
    const auto entry_ofs = entries[entry_n].entries[sub_n];
    return {entry_ofs, entry_ofs - sub_n * 8 - 4};
  }
  std::vector<TextureSamplerMapping> entries;
  std::map<std::pair<const Material*, int>, std::pair<int, int>> matMap;

  auto begin() { return entries.begin(); }
  auto end() { return entries.end(); }
  std::size_t size() const { return entries.size(); }
  TextureSamplerMapping& operator[](std::size_t i) { return entries[i]; }
  const TextureSamplerMapping& operator[](std::size_t i) const {
    return entries[i];
  }
};

void WriteMaterial(const size_t& mat_start, oishii::Writer& writer,
                   riistudio::g3d::NameTable& names,
                   const riistudio::g3d::Material& mat, u32& mat_idx,
                   riistudio::g3d::RelocWriter& linker,
                   const ShaderAllocator& shader_allocator,
                   TextureSamplerMappingManager& tex_sampler_mappings) {
  DebugReport("MAT_START %x\n", (u32)mat_start);
  DebugReport("MAT_NAME %x\n", writer.tell());
  writeNameForward(names, writer, mat_start, mat.IGCMaterial::getName());
  DebugReport("MATIDAT %x\n", writer.tell());
  writer.write<u32>(mat_idx++);
  u32 flag = mat.flag;
  flag = (flag & ~0x80000000) | (mat.xlu ? 0x80000000 : 0);
  writer.write<u32>(flag);
  WriteMaterialGenMode(writer, mat);
  // Misc
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
  using CommonTransformModel = libcube::GCMaterialData::CommonTransformModel;
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
        DebugReport("<material=\"%s\" sampler=%u>\n",
                    mat.IGCMaterial::getName().c_str(), i);
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
  writeMaterialDisplayList(mat.getMaterialData(), writer);
}
void WriteMesh(oishii::Writer& writer, const riistudio::g3d::Polygon& mesh,
               const riistudio::g3d::Model& mdl, const size_t& mesh_start,
               riistudio::g3d::NameTable& names) {
  const auto build_dlcache = [&]() {
    librii::gpu::DLBuilder dl(writer);
    writer.skip(5 * 2); // runtime
    dl.setVtxDescv(mesh.mVertexDescriptor);
    writer.write<u8>(0);
    // dl.align(); // 1
  };
  const auto build_dlsetup = [&]() {
    build_dlcache();
    librii::gpu::DLBuilder dl(writer);

    // Build desc
    // ----
    std::vector<
        std::pair<librii::gx::VertexAttribute, librii::gx::VQuantization>>
        desc;
    for (auto [attr, type] : mesh.getVcd().mAttributes) {
      if (type == librii::gx::VertexAttributeType::None)
        continue;
      librii::gx::VQuantization quant;

      const auto set_quant = [&](const auto& quantize) {
        quant.comp = quantize.mComp;
        quant.type = quantize.mType;
        quant.divisor = quantize.divisor;
        quant.bad_divisor = quantize.divisor;
        quant.stride = quantize.stride;
      };

      using VA = librii::gx::VertexAttribute;
      switch (attr) {
      case VA::Position: {
        const auto* buf = mdl.getBuf_Pos().findByName(mesh.mPositionBuffer);
        assert(buf);
        set_quant(buf->mQuantize);
        break;
      }
      case VA::Color0: {
        const auto* buf = mdl.getBuf_Clr().findByName(mesh.mColorBuffer[0]);
        assert(buf);
        set_quant(buf->mQuantize);
        break;
      }
      case VA::Color1: {
        const auto* buf = mdl.getBuf_Clr().findByName(mesh.mColorBuffer[1]);
        assert(buf);
        set_quant(buf->mQuantize);
        break;
      }
      case VA::TexCoord0:
      case VA::TexCoord1:
      case VA::TexCoord2:
      case VA::TexCoord3:
      case VA::TexCoord4:
      case VA::TexCoord5:
      case VA::TexCoord6:
      case VA::TexCoord7: {
        const auto chan =
            static_cast<int>(attr) - static_cast<int>(VA::TexCoord0);

        const auto* buf =
            mdl.getBuf_Uv().findByName(mesh.mTexCoordBuffer[chan]);
        assert(buf);
        set_quant(buf->mQuantize);
        break;
      }
      case VA::Normal:
      case VA::NormalBinormalTangent: {
        const auto* buf = mdl.getBuf_Nrm().findByName(mesh.mNormalBuffer);
        assert(buf);
        set_quant(buf->mQuantize);
        break;
      }
      default:
        break;
      }

      std::pair<librii::gx::VertexAttribute, librii::gx::VQuantization> tmp{
          attr, quant};
      desc.push_back(tmp);
    }
    // ---

    dl.setVtxAttrFmtv(0, desc);
    writer.skip(12 * 12); // array pointers set on runtime
    dl.align();           // 30
  };
  const auto assert_since = [&](u32 ofs) {
    MAYBE_UNUSED const auto since = writer.tell() - mesh_start;
    assert(since == ofs);
  };

  assert_since(8);
  writer.write<s32>(mesh.mCurrentMatrix);

  assert_since(12);
  const auto [c_a, c_b, c_c] =
      librii::gpu::DLBuilder::calcVtxDescv(mesh.mVertexDescriptor);
  writer.write<u32>(c_a);
  writer.write<u32>(c_b);
  writer.write<u32>(c_c);

  assert_since(24);
  DlHandle setup(writer);

  DlHandle data(writer);

  auto vcd = mesh.mVertexDescriptor;
  vcd.calcVertexDescriptorFromAttributeList();
  assert_since(0x30);
  writer.write<u32>(vcd.mBitfield);
  writer.write<u32>(static_cast<u32>(mesh.currentMatrixEmbedded) +
                    (static_cast<u32>(!mesh.isVisible()) << 1));
  writeNameForward(names, writer, mesh_start, mesh.getName());
  writer.write<u32>(mesh.mId);
  // TODO
  assert_since(0x40);
  const auto [nvtx, ntri] = librii::gx::ComputeVertTriCounts(mesh);
  writer.write<u32>(nvtx);
  writer.write<u32>(ntri);

  assert_since(0x48);
  const auto pos_idx = mdl.getBuf_Pos().indexOf(mesh.mPositionBuffer);
  writer.write<s16>(pos_idx);
  writer.write<s16>(mdl.getBuf_Nrm().indexOf(mesh.mNormalBuffer));
  for (int i = 0; i < 2; ++i)
    writer.write<s16>(mdl.getBuf_Clr().indexOf(mesh.mColorBuffer[i]));
  for (int i = 0; i < 8; ++i)
    writer.write<s16>(mdl.getBuf_Uv().indexOf(mesh.mTexCoordBuffer[i]));
  writer.write<s16>(-1); // fur
  writer.write<s16>(-1); // fur
  assert_since(0x64);
  writer.write<s32>(0x68); // msu, directly follows
  // TODO
  writer.write<u32>(0);
  writer.write<u32>(0);

  writer.alignTo(32);
  setup.setBufAddr(writer.tell());
  setup.setCmdSize(0xa0);
  setup.setBufSize(0xe0); // 0xa0 is already 32b aligned
  setup.write();
  build_dlsetup();

  writer.alignTo(32);
  data.setBufAddr(writer.tell());
  const auto data_start = writer.tell();
  {
    for (auto& mp : mesh.mMatrixPrimitives)
      writeVertexDataDL(mesh, mp, writer);
  }
  data.setCmdSize(writer.tell() - data_start);
  writer.alignTo(32);
  data.write();
}
void BuildRenderLists(const riistudio::g3d::Model& mdl,
                      std::vector<riistudio::g3d::RenderList>& renderLists) {
  RenderList nodeTree, drawOpa, drawXlu;
  nodeTree.name = "NodeTree";
  RenderList::DrawCmd root_bone_desc;
  root_bone_desc.type = RenderCommand::NodeDescendence;
  nodeTree.cmds.push_back(root_bone_desc);
  drawOpa.name = "DrawOpa";
  drawXlu.name = "DrawXlu";

  // Only supports one bone
  for (auto& draw : mdl.getBones()[0].mDisplayCommands) {
    RenderList::DrawCmd cmd;
    cmd.matIdx = draw.mMaterial;
    cmd.polyIdx = draw.mPoly;
    cmd.boneIdx = 0;
    cmd.prio = draw.mPrio;
    if (mdl.getMaterials()[draw.mMaterial].isXluPass())
      drawXlu.cmds.push_back(cmd);
    else
      drawOpa.cmds.push_back(cmd);
  }
  renderLists.push_back(nodeTree);
  if (!drawOpa.cmds.empty())
    renderLists.push_back(drawOpa);
  if (!drawXlu.cmds.empty())
    renderLists.push_back(drawXlu);
}
void writeModel(const Model& mdl, oishii::Writer& writer, RelocWriter& linker,
                NameTable& names, std::size_t brres_start) {
  const auto mdl_start = writer.tell();
  int d_cursor = 0;

  //
  // Build render lists
  //
  std::vector<RenderList> renderLists;
  BuildRenderLists(mdl, renderLists);

  //
  // Build shaders
  //

  ShaderAllocator shader_allocator;
  for (auto& mat : mdl.getMaterials()) {
    librii::g3d::G3dShader shader(mat.getMaterialData());
    shader_allocator.alloc(shader);
  }

  std::map<std::string, u32> Dictionaries;
  const auto write_dict = [&](const std::string& name, auto src_range,
                              auto handler, bool raw = false, u32 align = 4) {
    if (src_range.end() == src_range.begin())
      return;
    int d_pos = Dictionaries.at(name);
    writeDictionary<true, false>(name, src_range, handler, linker, writer,
                                 mdl_start, names, &d_pos, raw, align);
  };
  const auto write_dict_mat = [&](const std::string& name, auto src_range,
                                  auto handler, bool raw = false,
                                  u32 align = 4) {
    if (src_range.end() == src_range.begin())
      return;
    int d_pos = Dictionaries.at(name);
    writeDictionary<true, true>(name, src_range, handler, linker, writer,
                                mdl_start, names, &d_pos, raw, align);
  };

  {
    linker.label("MDL0");
    writer.write<u32>('MDL0');                  // magic
    linker.writeReloc<u32>("MDL0", "MDL0_END"); // file size
    writer.write<u32>(11);                      // revision
    writer.write<u32>(brres_start - mdl_start);
    // linker.writeReloc<s32>("MDL0", "BRRES");    // offset to the BRRES start

    // Section offsets
    linker.writeReloc<s32>("MDL0", "RenderTree");

    linker.writeReloc<s32>("MDL0", "Bones");
    linker.writeReloc<s32>("MDL0", "Buffer_Position");
    linker.writeReloc<s32>("MDL0", "Buffer_Normal");
    linker.writeReloc<s32>("MDL0", "Buffer_Color");
    linker.writeReloc<s32>("MDL0", "Buffer_UV");

    writer.write<s32>(0); // furVec
    writer.write<s32>(0); // furPos

    linker.writeReloc<s32>("MDL0", "Materials");
    linker.writeReloc<s32>("MDL0", "Shaders");
    linker.writeReloc<s32>("MDL0", "Meshes");
    linker.writeReloc<s32>("MDL0", "TexSamplerMap");
    writer.write<s32>(0); // PaletteSamplerMap
    writer.write<s32>(0); // UserData
    writeNameForward(names, writer, mdl_start, mdl.mName, true);
  }
  {
    linker.label("MDL0_INFO");
    writer.write<u32>(0x40); // Header size
    linker.writeReloc<s32>("MDL0_INFO", "MDL0");
    writer.write<u32>(static_cast<u32>(mdl.mScalingRule));
    writer.write<u32>(static_cast<u32>(mdl.mTexMtxMode));

    const auto [nVert, nTri] = computeVertTriCounts(mdl);
    writer.write<u32>(nVert);
    writer.write<u32>(nTri);

    writer.write<u32>(0); // mdl.sourceLocation

    // TODO Bad assumption, nViewMtx
    writer.write<u32>(mdl.getBones().size());
    // TODO bMtxArray, bTexMtxArray, bBoundVolume
    writer.write<u8>(1);
    writer.write<u8>(0);
    writer.write<u8>(0);

    writer.write<u8>(static_cast<u8>(mdl.mEvpMtxMode));
    writer.write<u32>(0x40); // offset to bone table, effectively header size
    mdl.aabb.min >> writer;
    mdl.aabb.max >> writer;
    // Bone table
    // TODO: -1 for non-singlebound? Out of current scope for now
    writer.write<u32>(mdl.getBones().size());
    for (int i = 0; i < mdl.getBones().size(); ++i)
      writer.write<u32>(i);
  }

  d_cursor = writer.tell();
  u32 dicts_size = 0;
  const auto tally_dict = [&](const char* str, const auto& dict) {
    DebugReport("%s: %u entries\n", str, (unsigned)dict.size());
    if (dict.empty())
      return;
    Dictionaries.emplace(str, dicts_size + d_cursor);
    dicts_size += 24 + 16 * dict.size();
  };
  tally_dict("RenderTree", renderLists);
  tally_dict("Bones", mdl.getBones());
  tally_dict("Buffer_Position", mdl.getBuf_Pos());
  tally_dict("Buffer_Normal", mdl.getBuf_Nrm());
  tally_dict("Buffer_Color", mdl.getBuf_Clr());
  tally_dict("Buffer_UV", mdl.getBuf_Uv());
  tally_dict("Materials", mdl.getMaterials());
  tally_dict("Shaders", mdl.getMaterials());
  tally_dict("Meshes", mdl.getMeshes());
  u32 n_samplers = 0;
  for (auto& mat : mdl.getMaterials())
    n_samplers += mat.samplers.size();
  if (n_samplers) {
    Dictionaries.emplace("TexSamplerMap", dicts_size + d_cursor);
    dicts_size += 24 + 16 * n_samplers;
  }
  for (auto [key, val] : Dictionaries) {
    DebugReport("%s: %x\n", key.c_str(), (unsigned)val);
  }
  writer.skip(dicts_size);

  TextureSamplerMappingManager tex_sampler_mappings;

  // for (auto& mat : mdl.getMaterials()) {
  //   for (int s = 0; s < mat.samplers.size(); ++s) {
  //     auto& samp = *mat.samplers[s];
  //     tex_sampler_mappings.add_entry(samp.mTexture);
  //   }
  // }
  // Matching order..
  for (auto& tex : dynamic_cast<const Collection*>(mdl.childOf)->getTextures())
    for (auto& mat : mdl.getMaterials())
      for (int s = 0; s < mat.samplers.size(); ++s)
        if (mat.samplers[s].mTexture == tex.getName())
          tex_sampler_mappings.add_entry(tex.getName(), &mat, s);

  int sm_i = 0;
  write_dict(
      "TexSamplerMap", tex_sampler_mappings,
      [&](TextureSamplerMapping& map, std::size_t start) {
        writer.write<u32>(map.entries.size());
        for (int i = 0; i < map.entries.size(); ++i) {
          tex_sampler_mappings.entries[sm_i].entries[i] = writer.tell();
          writer.write<s32>(0);
          writer.write<s32>(0);
        }
        ++sm_i;
      },
      true, 4);

  write_dict(
      "RenderTree", renderLists,
      [&](const RenderList& list, std::size_t) {
        WriteRenderList(list, writer);
      },
      true, 1);

  u32 bone_id = 0;
  write_dict("Bones", mdl.getBones(),
             [&](const Bone& bone, std::size_t bone_start) {
               DebugReport("Bone at %x\n", (unsigned)bone_start);
               WriteBone(names, writer, bone_start, bone, bone_id);
             });

  u32 mat_idx = 0;
  write_dict_mat(
      "Materials", mdl.getMaterials(),
      [&](const Material& mat, std::size_t mat_start) {
        linker.label("Mat" + std::to_string(mat_start), mat_start);
        WriteMaterial(mat_start, writer, names, mat, mat_idx, linker,
                      shader_allocator, tex_sampler_mappings);
      },
      false, 4);

  // Shaders
  {
    if (shader_allocator.size() == 0)
      return;

    u32 dict_ofs = Dictionaries.at("Shaders");
    librii::g3d::QDictionary _dict;
    _dict.mNodes.resize(mdl.getMaterials().size() + 1);

    for (std::size_t i = 0; i < shader_allocator.size(); ++i) {
      writer.alignTo(32); //
      for (int j = 0; j < shader_allocator.matToShaderMap.size(); ++j) {
        if (shader_allocator.matToShaderMap[j] == i) {
          _dict.mNodes[j + 1].setDataDestination(writer.tell());
          _dict.mNodes[j + 1].setName(
              mdl.getMaterials()[j].IGCMaterial::getName());
        }
      }
      const auto backpatch = writePlaceholder(writer); // size
      writer.write<s32>(mdl_start - backpatch);        // mdl0 offset
      WriteShader(linker, writer, shader_allocator.shaders[i], backpatch, i);
      writeOffsetBackpatch(writer, backpatch, backpatch);
    }
    {
      oishii::Jump<oishii::Whence::Set, oishii::Writer> d(writer, dict_ofs);
      linker.label("Shaders");
      _dict.write(writer, names);
    }
  }
  write_dict(
      "Meshes", mdl.getMeshes(),
      [&](const g3d::Polygon& mesh, std::size_t mesh_start) {
        WriteMesh(writer, mesh, mdl, mesh_start, names);
      },
      false, 32);

  write_dict(
      "Buffer_Position", mdl.getBuf_Pos(),
      [&](const PositionBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names);
      },
      false, 32);
  write_dict(
      "Buffer_Normal", mdl.getBuf_Nrm(),
      [&](const NormalBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names);
      },
      false, 32);
  write_dict(
      "Buffer_Color", mdl.getBuf_Clr(),
      [&](const ColorBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names);
      },
      false, 32);
  write_dict(
      "Buffer_UV", mdl.getBuf_Uv(),
      [&](const TextureCoordinateBuffer& buf, std::size_t buf_start) {
        writeGenericBuffer(buf, writer, buf_start, names);
      },
      false, 32);

  linker.writeChildren();
  linker.label("MDL0_END");
  linker.resolve();
} // namespace riistudio::g3d

} // namespace riistudio::g3d
