#pragma once

#include <core/common.h>
#include <core/util/oishii.hpp>
#include <librii/g3d/data/MaterialData.hpp>
#include <librii/g3d/io/CommonIO.hpp>
#include <librii/g3d/io/NameTableIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <librii/gpu/DLPixShader.hpp>
#include <rsl/SafeReader.hpp>
#include <rsl/SmallVector.hpp>

namespace librii::g3d {

struct TextureSamplerMapping {
  TextureSamplerMapping() = default;
  TextureSamplerMapping(std::string n, u32 pos) : name(n) {
    entries.emplace_back(pos);
  }

  const std::string& getName() const { return name; }
  std::string name;
  rsl::small_vector<u32, 1> entries; // stream position
};

struct BinaryMaterial;

struct TextureSamplerMappingManager {
  void add_entry(const std::string& name, const std::string& mat,
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
  bool contains(const std::string& name) const {
    auto found = std::find_if(entries.begin(), entries.end(),
                              [name](auto& e) { return e.name == name; });
    return found != entries.end();
  }
  std::pair<u32, u32> from_mat(const std::string& mat, int sampl_idx) {
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
  std::map<std::pair<std::string, int>, std::pair<int, int>> matMap;

  auto begin() { return entries.begin(); }
  auto end() { return entries.end(); }
  std::size_t size() const { return entries.size(); }
  TextureSamplerMapping& operator[](std::size_t i) { return entries[i]; }
  const TextureSamplerMapping& operator[](std::size_t i) const {
    return entries[i];
  }
};

bool readMaterial(G3dMaterialData& mat, oishii::BinaryReader& reader,
                  bool ignore_tev = false);

void WriteMaterialBody(size_t mat_start, oishii::Writer& writer,
                       NameTable& names, const G3dMaterialData& mat,
                       u32 mat_idx, RelocWriter& linker,
                       TextureSamplerMappingManager& tex_sampler_mappings);

// Bring into namespace
#include <core/util/glm_io.hpp>

inline void operator>>(const librii::gx::Color& out, oishii::Writer& writer) {
  librii::gx::writeColorComponents(writer, out,
                                   librii::gx::VertexBufferType::Color::rgba8);
}
struct BinaryGenMode {
  u8 numTexGens = 0;
  u8 numChannels = 0;
  u8 numTevStages = 0;
  u8 numIndStages = 0;
  librii::gx::CullMode cullMode = librii::gx::CullMode::None;

  Result<void> read(rsl::SafeReader& reader) {
    numTexGens = TRY(reader.U8());
    numChannels = TRY(reader.U8());
    numTevStages = TRY(reader.U8());
    numIndStages = TRY(reader.U8());
    cullMode = TRY(reader.Enum32<librii::gx::CullMode>());
    return {};
  }
  void write(oishii::Writer& writer) const {
    writer.write<u8>(numTexGens);
    writer.write<u8>(numChannels);
    writer.write<u8>(numTevStages);
    writer.write<u8>(numIndStages);
    writer.write<u32>(static_cast<u32>(cullMode));
  }
};

struct BinaryMatMisc {
  bool earlyZComparison = true;
  s8 lightSetIndex = -1;
  s8 fogIndex = -1;
  u8 reserved = 0;
  std::array<librii::g3d::G3dIndMethod, 4> indMethod{};
  std::array<s8, 4> normalMapLightIndices{-1, -1, -1, -1};
  Result<void> read(rsl::SafeReader& reader) {
    earlyZComparison = TRY(reader.U8());
    lightSetIndex = TRY(reader.S8());
    fogIndex = TRY(reader.S8());
    reserved = TRY(reader.U8());
    for (auto& m : indMethod)
      m = TRY(reader.Enum8<librii::g3d::G3dIndMethod>());
    for (auto& n : normalMapLightIndices)
      n = TRY(reader.S8());
    return {};
  }
  void write(oishii::Writer& writer) const {
    writer.write<u8>(earlyZComparison ? 1 : 0);
    writer.write<s8>(lightSetIndex);
    writer.write<s8>(fogIndex);
    writer.write<u8>(reserved);
    for (const auto& m : indMethod)
      writer.write<u8>(static_cast<u8>(m));
    for (const auto& n : normalMapLightIndices)
      writer.write<s8>(n);
  }
};

struct BinarySampler {
  std::string texture;
  std::string palette;
  u32 runtimeTexPtr = 0;
  u32 runtimePlttPtr = 0;
  u32 texMapId = 0; // Slot [0, 7]
  u32 tlutId = 0;
  librii::gx::TextureWrapMode wrapU = librii::gx::TextureWrapMode::Repeat;
  librii::gx::TextureWrapMode wrapV = librii::gx::TextureWrapMode::Repeat;
  librii::gx::TextureFilter minFilter = librii::gx::TextureFilter::near_mip_lin;
  librii::gx::TextureFilter magFilter = librii::gx::TextureFilter::Linear;
  f32 lodBias = 0.0f;
  librii::gx::AnisotropyLevel maxAniso;
  bool biasClamp = false;
  bool edgeLod = false;
  u16 reserved = 0;

  Result<void> read(rsl::SafeReader& reader) {
    const auto sampStart = reader.tell();
    texture = TRY(reader.StringOfs(sampStart));
    palette = TRY(reader.StringOfs(sampStart));
    runtimeTexPtr = TRY(reader.U32());
    runtimePlttPtr = TRY(reader.U32());
    texMapId = TRY(reader.U32());
    tlutId = TRY(reader.U32());
    wrapU = TRY(reader.Enum32<librii::gx::TextureWrapMode>());
    wrapV = TRY(reader.Enum32<librii::gx::TextureWrapMode>());
    minFilter = TRY(reader.Enum32<librii::gx::TextureFilter>());
    magFilter = TRY(reader.Enum32<librii::gx::TextureFilter>());
    lodBias = TRY(reader.F32());
    maxAniso = TRY(reader.Enum32<librii::gx::AnisotropyLevel>());
    biasClamp = TRY(reader.U8());
    edgeLod = TRY(reader.U8());
    reserved = TRY(reader.U16());
    return {};
  }
  void write(oishii::Writer& writer, NameTable& names, u32 nameOfs) const {
    writeNameForward(names, writer, nameOfs, texture);
    writeNameForward(names, writer, nameOfs, palette);
    writer.write<u32>(runtimeTexPtr);
    writer.write<u32>(runtimePlttPtr);
    writer.write<u32>(texMapId);
    writer.write<u32>(tlutId);
    writer.write<u32>(static_cast<u32>(wrapU));
    writer.write<u32>(static_cast<u32>(wrapV));
    writer.write<u32>(static_cast<u32>(minFilter));
    writer.write<u32>(static_cast<u32>(magFilter));
    writer.write<f32>(lodBias);
    writer.write<u32>(static_cast<u32>(maxAniso));
    writer.write<u8>(biasClamp ? 1 : 0);
    writer.write<u8>(edgeLod ? 1 : 0);
    writer.write<u16>(reserved);
  }
};

struct BinaryMatDL {
  // Pixel
  gx::AlphaComparison alphaCompare{};
  gx::ZMode zMode{};
  gx::BlendMode blendMode{};
  gx::DstAlpha dstAlpha{};

  // TEV Color
  std::array<gx::ColorS10, 3> tevColors{}; // Skip CPREV/APREV
  std::array<gx::Color, 4> tevKonstColors{};

  // Indirect
  std::array<librii::gx::IndirectTextureScalePair, 4> scales{};
  rsl::array_vector<gx::IndirectMatrix, 3> indMatrices{};

  // TexGen
  rsl::array_vector<gx::TexCoordGen, 8> texGens{};

  // Writes in the standard, fixed order
  void write(oishii::Writer& writer) const;
  // Actually parses the display list commands in a GPU emulator.
  Result<void> parse(oishii::BinaryReader& reader, u32 numIndStages,
                     u32 numTexGens);
};

struct BinaryTexSrt {
  glm::vec2 scale = glm::vec2(1.0f, 1.0f);
  f32 rotate_degrees = 0.0f;
  glm::vec2 translate = glm::vec2(0.0f, 0.0f);

  Result<void> read(rsl::SafeReader& reader) {
    scale.x = TRY(reader.F32());
    scale.y = TRY(reader.F32());
    rotate_degrees = TRY(reader.F32());
    translate.x = TRY(reader.F32());
    translate.y = TRY(reader.F32());
    return {};
  }
  void write(oishii::Writer& writer) const {
    scale >> writer;
    writer.write<f32>(rotate_degrees);
    translate >> writer;
  }
};

struct BinaryTexMtxEffect {
  enum {
    EFFECT_MTX_IDENTITY = (1 << 0),
  };

  s8 camIdx = -1;
  s8 lightIdx = -1;
  u8 mapMode = 0;
  u8 flag = EFFECT_MTX_IDENTITY;
  // G3D effectmatrix is 3x4. J3D is 4x4
  glm::mat3x4 effectMtx{1.0f};

  Result<void> read(rsl::SafeReader& reader) {
    camIdx = TRY(reader.S8());
    lightIdx = TRY(reader.S8());
    mapMode = TRY(reader.U8());
    flag = TRY(reader.U8());
    for (size_t i = 0; i < 3; ++i)
      for (size_t j = 0; j < 4; ++j)
        effectMtx[i][j] = TRY(reader.F32());
    return {};
  }
  void write(oishii::Writer& writer) const {
    writer.write<s8>(camIdx);
    writer.write<s8>(lightIdx);
    writer.write<u8>(mapMode);
    writer.write<u8>(flag);
    for (size_t i = 0; i < 3; ++i)
      for (size_t j = 0; j < 4; ++j)
        writer.write<f32>(effectMtx[i][j]);
  }
};

struct BinaryTexSrtData {
  u32 flag = 0;
  librii::g3d::TexMatrixMode texMtxMode = librii::g3d::TexMatrixMode::Maya;
  std::array<BinaryTexSrt, 8> srt{};
  std::array<BinaryTexMtxEffect, 8> effect{};

  Result<void> read(rsl::SafeReader& reader) {
    flag = TRY(reader.U32());
    texMtxMode = TRY(reader.Enum32<librii::g3d::TexMatrixMode>());
    for (auto& s : srt)
      TRY(s.read(reader));
    for (auto& e : effect)
      TRY(e.read(reader));
    return {};
  }
  void write(oishii::Writer& writer) const {
    writer.write<u32>(flag);
    writer.write<u32>(static_cast<u32>(texMtxMode));
    for (auto& s : srt)
      s.write(writer);
    for (auto& e : effect)
      e.write(writer);
  }
};

struct BinaryChannelData {
  enum {
    GDSetChanMatColor_COLOR0 = 0x1,
    GDSetChanMatColor_ALPHA0 = 0x2,
    GDSetChanAmbColor_COLOR0 = 0x4,
    GDSetChanAmbColor_ALPHA0 = 0x8,
    GDSetChanCtrl_COLOR0 = 0x10,
    GDSetChanCtrl_ALPHA0 = 0x20,
  };
  struct Channel {
    u32 flag = 0;
    librii::gx::Color material;
    librii::gx::Color ambient;
    librii::gpu::LitChannel xfCntrlColor;
    librii::gpu::LitChannel xfCntrlAlpha;

    Result<void> read(rsl::SafeReader& reader) {
      flag = TRY(reader.U32());
      material.r = TRY(reader.U8());
      material.g = TRY(reader.U8());
      material.b = TRY(reader.U8());
      material.a = TRY(reader.U8());
      ambient.r = TRY(reader.U8());
      ambient.g = TRY(reader.U8());
      ambient.b = TRY(reader.U8());
      ambient.a = TRY(reader.U8());
      xfCntrlColor.hex = TRY(reader.U32());
      xfCntrlAlpha.hex = TRY(reader.U32());
      return {};
    }

    void write(oishii::Writer& writer) const {
      writer.write<u32>(flag);
      material >> writer;
      ambient >> writer;
      writer.write<u32>(xfCntrlColor.hex);
      writer.write<u32>(xfCntrlAlpha.hex);
    }
  };

  std::array<Channel, 2> chan;

  Result<void> read(rsl::SafeReader& reader) {
    for (auto& c : chan) {
      TRY(c.read(reader));
    }
    return {};
  }
  void write(oishii::Writer& reader) const {
    for (auto& c : chan) {
      c.write(reader);
    }
  }
};

struct BinaryMaterial {
  std::string name;
  u32 id;
  u32 flag;

  BinaryGenMode genMode;
  BinaryMatMisc misc;
  u32 tevId = 0;

  std::vector<BinarySampler> samplers;

  BinaryMatDL dl;
  std::array<u8, (4 + 8 * 12) + (4 + 8 * 32)> texPlttRuntimeObjs = {};
  BinaryTexSrtData texSrtData;

  BinaryChannelData chan;

  // HACK: Until BinaryTev
  gx::LowLevelGxMaterial tev;

  Result<void> read(oishii::BinaryReader& reader,
                    gx::LowLevelGxMaterial* outShader);
  // TODO: Reduce dependencies
  void writeBody(oishii::Writer& writer, u32 mat_start, NameTable& names,
                 RelocWriter& linker,
                 TextureSamplerMappingManager& tex_sampler_mappings) const;
};

Result<G3dMaterialData> fromBinMat(const BinaryMaterial& bin,
                                   gx::LowLevelGxMaterial* shader);
BinaryMaterial toBinMat(const G3dMaterialData& mat, u32 mat_idx);

} // namespace librii::g3d
