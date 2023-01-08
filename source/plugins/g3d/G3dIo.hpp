#pragma once

#include "collection.hpp"

namespace riistudio::g3d {

void ReadBRRES(Collection& collection, oishii::BinaryReader& reader,
               kpi::LightIOTransaction& transaction);
Result<void> WriteBRRES(Collection& collection, oishii::Writer& writer);

} // namespace riistudio::g3d
