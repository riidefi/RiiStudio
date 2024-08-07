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

struct ChrAnim;
struct SrtAnim;
struct BinaryVis;
struct PatAnim;
struct ClrAnim;

struct BinaryArchive;

struct Archive {
  std::vector<Model> models;
  std::vector<TextureData> textures;
  std::vector<ChrAnim> chrs;
  std::vector<ClrAnim> clrs;
  std::vector<PatAnim> pats;
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
  Result<std::vector<u8>> write() const;
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
