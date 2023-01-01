#pragma once

#include <core/common.h>
#include <core/util/oishii.hpp>
#include <librii/g3d/data/MaterialData.hpp>

namespace librii::g3d {

struct BinaryTevDL {
  gx::SwapTable swapTable{};
  rsl::array_vector<gx::IndOrder, 4> indOrders{};
  rsl::array_vector<gx::TevStage, 16> tevStages{};

  void write(oishii::Writer& writer) const;
};

struct BinaryTev {
  u32 id{};
  // Disabled to simplify
  // u8 numStages{};
  std::array<u8, 3> reserved{};
  std::array<u8, 8> coordToMapLut{};
  std::array<u8, 8> reserved2{};
  BinaryTevDL dl{};
  void read(oishii::BinaryReader& reader, unsigned int tev_addr,
            bool brawlbox_bug = false);
  void writeBody(oishii::Writer& writer) const;
};

BinaryTev toBinaryTev(const G3dShader& sh, u32 tev_id);

Result<void> ReadTev(librii::gx::LowLevelGxMaterial& mat,
                     oishii::BinaryReader& reader, unsigned int tev_addr,
                     bool trust_stagecount = false, bool brawlbox_bug = false);

void WriteTevBody(oishii::Writer& writer, u32 tev_id, const G3dShader& tev);

} // namespace librii::g3d
