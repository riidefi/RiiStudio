#pragma once

#include "collection.hpp"

namespace riistudio::g3d {

[[nodiscard]] Result<void> ReadBRRES(Collection& collection,
                                     oishii::BinaryReader& reader,
                                     kpi::LightIOTransaction& transaction);
[[nodiscard]] Result<void> WriteBRRES(Collection& collection,
                                      oishii::Writer& writer);

} // namespace riistudio::g3d
