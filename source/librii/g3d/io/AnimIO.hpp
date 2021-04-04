#pragma once

#include <core/common.h>
#include <librii/g3d/data/AnimData.hpp>
#include <span>

namespace librii::g3d {

bool ReadSrtFile(SrtAnimationArchive& arc, std::span<const u8> data);

} // namespace librii::g3d
