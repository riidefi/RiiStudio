#pragma once

#include <core/common.h>
#include <core/util/oishii.hpp>
#include <oishii/writer/binary_writer.hxx>
#include <rsl/SafeReader.hpp>

namespace librii::egg {

enum class BaseLayer : u8 { SphereMap, Square, SphereMap2 };

enum class cPttrn : u8 {
  Linear32_64,
  Linear0_64,
  Linear16_64,
  Quadratic32_64,
  Quartic32_64,
  Sextic32_64,
  Octic32_64,
  Linear4_45,
  RevQuadratic10_42,
  RevQuad13_45,
};

struct DrawSetting {
  f32 normEffectScale{1.0f};
  cPttrn pattern{cPttrn::Linear32_64};
  // Stack junk: Entries past first 2 usually have this set to nonzero bytes
  std::array<u8, 3> stackjunk{};

  bool operator==(const DrawSetting&) const = default;
};

struct LightTexture {
  std::array<u8, 7> reserved0{};
  std::array<u8, 2> reserved1{};
  std::array<char, 32> textureName{};
  BaseLayer baseLayer{};
  std::array<u8, 3> reserved2{};
  u32 activeDrawSettings{};
  std::vector<DrawSetting> drawSettings{};

  bool operator==(const LightTexture&) const = default;

  u32 fileSize() const { return 0x3C + 8 * drawSettings.size(); }

  Result<void> read(rsl::SafeReader& reader);
  void write(oishii::Writer& writer) const;
};

struct LightMap {
  std::array<u8, 7> reserved0{};
  std::vector<LightTexture> textures{};
  std::array<u8, 14> reserved1{};

  bool operator==(const LightMap&) const = default;

  u32 fileSize() const {
    return std::accumulate(
        textures.begin(), textures.end(), 0x20,
        [](u32 accum, auto& tex) { return accum + tex.fileSize(); });
  }

  Result<void> read(rsl::SafeReader& reader);
  void write(oishii::Writer& writer) const;
};
Result<librii::egg::LightMap> ReadBlmap(std::span<const u8> buf,
                                        std::string_view path);
void WriteBlmap(const librii::egg::LightMap& b, std::string_view path);

} // namespace librii::egg
