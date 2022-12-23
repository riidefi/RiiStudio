#pragma once

#include <librii/g3d/data/AnimData.hpp>
#include <librii/g3d/data/TextureData.hpp>
#include <librii/g3d/io/AnimClrIO.hpp>
#include <librii/g3d/io/AnimTexPatIO.hpp>
#include <librii/g3d/io/ModelIO.hpp>
#include <vector>

// Regrettably, for kpi::LightIOTransaction
#include <core/kpi/Plugins.hpp>

namespace librii::g3d {

struct BinaryArchive {
  std::vector<librii::g3d::BinaryModel> models;
  std::vector<librii::g3d::TextureData> textures;
  std::vector<librii::g3d::BinaryClr> clrs;
  std::vector<librii::g3d::BinaryTexPat> pats;
  std::vector<librii::g3d::SrtAnimationArchive> srts;

  void read(oishii::BinaryReader& reader, kpi::LightIOTransaction& transaction);
  void write(oishii::Writer& writer);
};

} // namespace librii::g3d
