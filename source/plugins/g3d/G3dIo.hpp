#pragma once

#include "collection.hpp"

namespace librii::g3d {
struct Archive;
}

namespace riistudio::g3d {

Result<void> ReadBRRES(Collection& collection, librii::g3d::Archive& archive,
                       std::string path);
[[nodiscard]] Result<void> ReadBRRES(Collection& collection,
                                     oishii::BinaryReader& reader,
                                     kpi::LightIOTransaction& transaction);
[[nodiscard]] Result<void> WriteBRRES(Collection& collection,
                                      oishii::Writer& writer);

} // namespace riistudio::g3d
