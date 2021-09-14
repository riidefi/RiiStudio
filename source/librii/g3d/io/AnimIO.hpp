#pragma once

#include <core/common.h>
#include <core/util/oishii.hpp>
#include <librii/g3d/data/AnimData.hpp>
#include <librii/g3d/io/NameTableIO.hpp>
#include <span>

namespace librii::g3d {

bool ReadSrtFile(SrtAnimationArchive& arc, std::span<const u8> data);

void WriteSrtFile(oishii::Writer& writer, const SrtAnimationArchive& arc,
                  NameTable& names, u32 brres_pos);

} // namespace librii::g3d
