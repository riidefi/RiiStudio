#pragma once

#include <core/common.h>
#include <core/util/oishii.hpp>
#include <librii/g3d/data/MaterialData.hpp>

namespace librii::g3d {

void ReadTev(librii::gx::LowLevelGxMaterial& mat, oishii::BinaryReader& reader,
             unsigned int tev_addr);

void WriteTevBody(oishii::Writer& writer, u32 tev_id, const G3dShader& tev);

} // namespace librii::g3d
