#pragma once

#include <core/common.h>
#include <core/util/oishii.hpp>
#include <librii/g3d/data/MaterialData.hpp>
#include <librii/g3d/io/CommonIO.hpp>
#include <librii/g3d/io/NameTableIO.hpp>
#include <librii/g3d/io/TevIO.hpp>
#include <librii/gpu/DLPixShader.hpp>
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

inline void operator<<(librii::gx::Color& out, oishii::BinaryReader& reader) {
  out = librii::gx::readColorComponents(
      reader, librii::gx::VertexBufferType::Color::rgba8);
}

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

  void read(oishii::BinaryReader& reader) {
    numTexGens = reader.read<u8>();
    numChannels = reader.read<u8>();
    numTevStages = reader.read<u8>();
    numIndStages = reader.read<u8>();
    cullMode = static_cast<librii::gx::CullMode>(reader.read<u32>());
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
  void read(oishii::BinaryReader& reader) {
    earlyZComparison = reader.read<u8>();
    lightSetIndex = reader.read<s8>();
    fogIndex = reader.read<s8>();
    reserved = reader.read<u8>();
    for (auto& m : indMethod)
      m = static_cast<librii::g3d::G3dIndMethod>(reader.read<u8>());
    for (auto& n : normalMapLightIndices)
      n = reader.read<s8>();
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

  void read(oishii::BinaryReader& reader) {
    const auto sampStart = reader.tell();
    texture = readName(reader, sampStart);
    palette = readName(reader, sampStart);
    runtimeTexPtr = reader.read<u32>();
    runtimePlttPtr = reader.read<u32>();
    texMapId = reader.read<u32>();
    tlutId = reader.read<u32>();
    wrapU = static_cast<librii::gx::TextureWrapMode>(reader.read<u32>());
    wrapV = static_cast<librii::gx::TextureWrapMode>(reader.read<u32>());
    minFilter = static_cast<librii::gx::TextureFilter>(reader.read<u32>());
    magFilter = static_cast<librii::gx::TextureFilter>(reader.read<u32>());
    lodBias = reader.read<f32>();
    maxAniso = static_cast<librii::gx::AnisotropyLevel>(reader.read<u32>());
    biasClamp = reader.read<u8>();
    edgeLod = reader.read<u8>();
    reserved = reader.read<u16>();
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
  void parse(oishii::BinaryReader& reader, u32 numIndStages, u32 numTexGens);
};

struct BinaryTexSrt {
  glm::vec2 scale = glm::vec2(1.0f, 1.0f);
  f32 rotate_degrees = 0.0f;
  glm::vec2 translate = glm::vec2(0.0f, 0.0f);

  void read(oishii::BinaryReader& reader) {
    scale << reader;
    rotate_degrees = reader.read<f32>();
    translate << reader;
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

  void read(oishii::BinaryReader& reader) {
    camIdx = reader.read<s8>();
    lightIdx = reader.read<s8>();
    mapMode = reader.read<u8>();
    flag = reader.read<u8>();
    for (size_t i = 0; i < 3; ++i)
      for (size_t j = 0; j < 4; ++j)
        effectMtx[i][j] = reader.read<f32>();
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

  void read(oishii::BinaryReader& reader) {
    flag = reader.read<u32>();
    texMtxMode = static_cast<librii::g3d::TexMatrixMode>(reader.read<u32>());
    for (auto& s : srt)
      s.read(reader);
    for (auto& e : effect)
      e.read(reader);
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

    void read(oishii::BinaryReader& reader) {
      flag = reader.read<u32>();
      material << reader;
      ambient << reader;
      xfCntrlColor.hex = reader.read<u32>();
      xfCntrlAlpha.hex = reader.read<u32>();
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

  void read(oishii::BinaryReader& reader) {
    for (auto& c : chan) {
      c.read(reader);
    }
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

  void read(oishii::BinaryReader& reader, gx::LowLevelGxMaterial* outShader);
  // TODO: Reduce dependencies
  void writeBody(oishii::Writer& writer, u32 mat_start, NameTable& names,
                 RelocWriter& linker,
                 TextureSamplerMappingManager& tex_sampler_mappings) const;
};

G3dMaterialData fromBinMat(const BinaryMaterial& bin,
                           gx::LowLevelGxMaterial* shader);
BinaryMaterial toBinMat(const G3dMaterialData& mat, u32 mat_idx);

} // namespace librii::g3d
