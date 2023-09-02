#pragma once

#include <core/common.h>
#include <librii/g3d/io/CommonIO.hpp>
#include <span>
#include <string_view>

namespace librii::g3d {

struct TextureData;

bool ReadTexture(TextureData& tex, std::span<const u8> data,
                 std::string_view name);

BlockData CalcTextureBlockData(const TextureData& tex);

bool WriteTexture(std::span<u8> data, const TextureData& tex, s32 brres_ofs,
                  NameReloc& out_reloc);

//! A "TEX0" file is effectively a .brtex archive without the enclosing
//! structure.
//!
[[nodiscard]] Result<g3d::TextureData> ReadTEX0(std::span<const u8> file);

[[nodiscard]] std::vector<u8> WriteTEX0(const g3d::TextureData& tex);

} // namespace librii::g3d
