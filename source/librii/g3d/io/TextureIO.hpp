#pragma once

#include <core/common.h>
#include <span>
#include <string_view>

namespace librii::g3d {

struct TextureData;

bool ReadTexture(librii::g3d::TextureData& tex, std::span<const u8> data,
                 std::string_view name);

} // namespace librii::g3d
