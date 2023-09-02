#pragma once

#include "AnimData.hpp"
#include "ModelData.hpp"
#include "TextureData.hpp"
#include <rsl/Result.hpp>
#include <string_view>

namespace kpi {
struct LightIOTransaction;
}

namespace librii::g3d {

struct BinaryChr;
struct SrtAnim;
struct BinaryVis;
struct BinaryTexPat;
struct BinaryClr;

struct BinaryArchive;

struct Archive {
  std::vector<Model> models;
  std::vector<TextureData> textures;
  std::vector<librii::g3d::BinaryChr> chrs;
  std::vector<BinaryClr> clrs;
  std::vector<BinaryTexPat> pats;
  std::vector<librii::g3d::SrtAnim> srts;
  std::vector<librii::g3d::BinaryVis> viss;

  static Result<Archive> fromFile(std::string path,
                                  kpi::LightIOTransaction& transaction);
  static Result<Archive> fromFile(std::string path);
  static Result<Archive> fromMemory(std::span<const u8> buf, std::string path,
                                    kpi::LightIOTransaction& transaction);
  static Result<Archive> fromMemory(std::span<const u8> buf, std::string path);
  static Result<Archive> read(oishii::BinaryReader& reader,
                              kpi::LightIOTransaction& transaction);
  static Result<Archive> from(const BinaryArchive& model,
                              kpi::LightIOTransaction& transaction);
  Result<void> write(oishii::Writer& writer) const;
  Result<void> write(std::string_view path) const;
  Result<BinaryArchive> binary() const;

#ifndef ARCHIVE_DEF
  // Allows us to forward declare the AnimIO stuff
  Archive();
  Archive(const Archive&);
  Archive(Archive&&) noexcept;
  ~Archive();
#endif
};

} // namespace librii::g3d
