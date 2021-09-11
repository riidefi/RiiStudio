#pragma once

#include "Scene.hpp"
#include <core/kpi/Plugins.hpp>

namespace riistudio::j3d {

void WriteBMD(riistudio::j3d::Collection& collection, oishii::Writer& writer);
void ReadBMD(riistudio::j3d::Model& mdl, riistudio::j3d::Collection& collection,
             oishii::BinaryReader& reader,
             kpi::LightIOTransaction& transaction);

} // namespace riistudio::j3d
