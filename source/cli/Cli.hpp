#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

// cli.exe import <from> [to]
// --verbose
// --scale 1.0
// --brawlbox_scale
// --mipmaps off --mipmaps 32:5
// --auto_transparency
// --merge_mats
// --bake_uvs
// --tint #HEXCODE
// Mesh
// --cull_degenerates
// --cull_invalid
// --recompute_normals off
// --fuse_vertices on
//
using bool32 = uint32_t;

enum {
  TYPE_UNK,

  // Via Assimp
  TYPE_IMPORT_BRRES,

  // SZS
  TYPE_DECOMPRESS,
  TYPE_COMPRESS,

  TYPE_COMPILE_RHST_BRRES,
  TYPE_COMPILE_RHST_BMD,
};

template <size_t L> struct CFixedString {
  std::string_view view() const { return {buf, strnlen(buf, sizeof(buf))}; }
  char buf[L]{};
};
static_assert(sizeof(CFixedString<256>) == 256);

struct CliOptions {
  uint32_t type = 0;
  CFixedString<256> from;
  CFixedString<256> to;
  CFixedString<256> preset_path;
  float scale = 1.0f;
  bool32 brawlbox_scale = false;
  bool32 mipmaps = true;
  uint32_t min_mip = 32;
  uint32_t max_mips = 5;
  bool32 auto_transparency = true;
  bool32 merge_mats = true;
  bool32 bake_uvs = false;
  uint32_t hexcode = 0xFFFF'FFFF;
  bool32 cull_degenerates = true;
  bool32 cull_invalid = true;
  bool32 recompute_normals = false;
  bool32 fuse_vertices = true;
  bool32 no_tristrip = false;
  bool32 ai_json = false;
  bool32 verbose = false;
};

std::optional<CliOptions> parse(int argc, const char** argv);
