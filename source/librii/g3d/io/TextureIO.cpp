#include "TextureIO.hpp"
#include <librii/g3d/data/TextureData.hpp>
#include <rsl/SimpleReader.hpp>

namespace librii::g3d {

//! In this case we will not automatically manage the LOD bounds
static bool IsCustomLodConfig(const librii::g3d::TextureData& tex) {
  return tex.minLod != 0.0f ||
         tex.maxLod != static_cast<float>(tex.number_of_images - 1);
}

bool ReadTexture(librii::g3d::TextureData& tex, std::span<const u8> data,
                 std::string_view name) {
  // Verify reads up to +0x34
  if (data.size_bytes() < 0x34) {
    return false;
  }

  tex.name = name;

  {
    using namespace rsl::pp;
    unsigned cursor = 0;

    if (const auto fourcc = lwzu(data, cursor); fourcc != 'TEX0') {
      return false;
    }
    cursor += 4; // SKIP: Size
    if (const auto revision = lwzu(data, cursor);
        revision != 1 && revision != 3) {
      return false;
    }
    cursor += 4; // SKIP: BRRES offset
    cursor += 4; // DELAY: ofs_tex
    cursor += 4; // SKIP: Name offset
    cursor += 4; // SKIP: Flag
    tex.width = lhzu(data, cursor);
    tex.height = lhzu(data, cursor);
    tex.format = static_cast<librii::gx::TextureFormat>(lwzu(data, cursor));
    tex.number_of_images = lwzu(data, cursor);
    tex.minLod = lfsu(data, cursor);
    tex.maxLod = lfsu(data, cursor);
    tex.custom_lod = IsCustomLodConfig(tex);
    cursor += 4; // SKIP

    assert(cursor == 0x34);

    // cursor += 4; // SKIP (v3)
  }

  // Verify the image data can be read
  const u32 image_size = librii::gx::computeImageSize(
      tex.width, tex.height, tex.format, tex.number_of_images);

  const u32 ofs_tex = rsl::pp::lwz(data, 0x10);

  if (data.size_bytes() < ofs_tex + image_size) {
    return false;
  }

  tex.data.assign(data.data() + ofs_tex, data.data() + ofs_tex + image_size);

  return true;
}

} // namespace librii::g3d
