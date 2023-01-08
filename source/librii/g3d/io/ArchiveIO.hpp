#pragma once

#include <librii/g3d/data/AnimData.hpp>
#include <librii/g3d/data/TextureData.hpp>
#include <librii/g3d/io/AnimClrIO.hpp>
#include <librii/g3d/io/AnimIO.hpp>
#include <librii/g3d/io/AnimTexPatIO.hpp>
#include <librii/g3d/io/AnimVisIO.hpp>
#include <librii/g3d/io/ModelIO.hpp>

#include <expected>
#include <vector>

// Regrettably, for kpi::LightIOTransaction
#include <core/kpi/Plugins.hpp>

namespace librii::g3d {

struct BinaryArchive {
  std::vector<librii::g3d::BinaryModel> models;
  std::vector<librii::g3d::TextureData> textures;
  std::vector<librii::g3d::BinaryClr> clrs;
  std::vector<librii::g3d::BinaryTexPat> pats;
  std::vector<librii::g3d::BinarySrt> srts;
  std::vector<librii::g3d::BinaryVis> viss;

  Result<void> read(oishii::BinaryReader& reader,
                    kpi::LightIOTransaction& transaction);
  Result<void> write(oishii::Writer& writer);
};
struct Archive {
  std::vector<Model> models;
  std::vector<TextureData> textures;
  std::vector<BinaryClr> clrs;
  std::vector<BinaryTexPat> pats;
  std::vector<librii::g3d::BinarySrt> srts;
  std::vector<librii::g3d::BinaryVis> viss;

  static Result<Archive> from(const BinaryArchive& model,
                              kpi::LightIOTransaction& transaction);
  Result<BinaryArchive> binary() const;
};

} // namespace librii::g3d
