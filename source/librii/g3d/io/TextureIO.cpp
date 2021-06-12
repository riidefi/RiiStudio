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

BlockData CalcTextureBlockData(const TextureData& tex) {
  return {.size = 64 + ComputeImageSize(tex), .start_align = 32};
}

bool WriteTexture(std::span<u8> data, const TextureData& tex, s32 brres_ofs,
                  NameReloc& out_reloc) {
  const auto block = CalcTextureBlockData(tex);

  if (block.size < data.size_bytes())
    return false;

  rsl::store<u32>('TEX0', data, 0);
  rsl::store<u32>(block.size, data, 4);
  rsl::store<u32>(3, data, 8);          // revision
  rsl::store<u32>(brres_ofs, data, 12); // brres offset
  rsl::store<u32>(64, data, 16);        // texture offset

  rsl::store<u32>(~0, data, 20);
  out_reloc = {.offset_of_delta_reference = 0,
               .offset_of_pointer_in_struct = 20,
               .name = tex.name,
               .non_volatile = true};

  rsl::store<u32>(0, data, 24); // flag, ci
  rsl::store<u16>(tex.width, data, 28);
  rsl::store<u16>(tex.height, data, 30);
  rsl::store<u32>(static_cast<u32>(tex.format), data, 32);
  rsl::store<u32>(tex.number_of_images, data, 36);
  rsl::store<f32>(tex.custom_lod ? tex.minLod : 0.0f, data, 40);
  rsl::store<f32>(tex.custom_lod ? tex.maxLod
                                 : static_cast<f32>(tex.number_of_images - 1),
                  data, 44);

  // TODO: Trash data here in old version?
  // 0x40 << 24 output.. sometimes
  //
  // This doesn't have that bug
  rsl::store<u32>(0, data, 44); // src path

  rsl::store<u32>(0, data, 48); // user data

  // align
  for (int i = 52; i < 64; i += sizeof(u32)) {
    rsl::store<u32>(0, data, i);
  }

  const u8* begin = tex.data.data();
  const u8* end = begin + ComputeImageSize(tex);

  // Checked above
  std::memcpy(data.subspan(64).data(), tex.data.data(), end - begin);

  return true;
}

} // namespace librii::g3d
