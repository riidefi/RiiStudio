#include "TextureIO.hpp"
#include <librii/g3d/data/TextureData.hpp>
#include <rsl/SimpleReader.hpp>
#include <rsl/WriteFile.hpp>

namespace librii::g3d {

//! In this case we will not automatically manage the LOD bounds
static bool IsCustomLodConfig(const librii::g3d::TextureData& tex) {
  return tex.minLod != 0.0f ||
         tex.maxLod != static_cast<float>(tex.number_of_images - 1);
}

static float GetLodMinConfig(const librii::g3d::TextureData& tex) {
  return tex.custom_lod ? tex.minLod : 0.0f;
}

static float GetLodMaxConfig(const librii::g3d::TextureData& tex) {
  return tex.custom_lod ? tex.maxLod
                        : static_cast<float>(tex.number_of_images - 1);
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
      // return false;
    }
    cursor += 4; // SKIP: Size
    if (const auto revision = lwzu(data, cursor);
        revision != 1 && revision != 3) {
      return false;
    }
    cursor += 4; // SKIP: BRRES offset
    cursor += 4; // DELAY: ofs_tex
    if (tex.name.empty()) {
      auto name = ReadStringPointer(data, cursor, 0);
      if (!name) {
        return false;
      }
      tex.name = *name;
      cursor += 4;
    } else {
      cursor += 4; // SKIP: Name offset
    }
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
  rsl::store<f32>(GetLodMinConfig(tex), data, 40);
  rsl::store<f32>(GetLodMaxConfig(tex), data, 44);

  rsl::store<u32>(0, data, 48); // src path

  rsl::store<u32>(0, data, 52); // user data

  // align
  for (int i = 56; i < 64; i += sizeof(u32)) {
    rsl::store<u32>(0, data, i);
  }

  const u8* begin = tex.data.data();
  const u8* end = begin + ComputeImageSize(tex);

  // Checked above
  std::memcpy(data.subspan(64, end - begin).data(), tex.data.data(),
              end - begin);

  return true;
}

Result<g3d::TextureData, std::string> ReadTEX0(std::span<const u8> file) {
  g3d::TextureData tex;
  // TODO: We trust the .tex0-file provided name to be correct
  const bool ok = g3d::ReadTexture(tex, file, "");
  if (!ok) {
    return std::unexpected(
        "Failed to parse TEX0: g3d::ReadTexture returned false");
  }
  return tex;
}

class SimpleRelocApplier {
public:
  SimpleRelocApplier(std::vector<u8>& buf) : mBuf(buf) {}
  ~SimpleRelocApplier() = default;

  void apply(g3d::NameReloc reloc, u32 structure_offset) {
    // Transform from structure-space to buffer-space
    reloc = g3d::RebasedNameReloc(reloc, structure_offset);
    // Write to end of mBuffer
    const u32 string_location = writeName(reloc.name);
    mBuf.resize(roundUp(mBuf.size(), 4));
    // Resolve pointer
    writePointer(reloc, string_location);
  }

private:
  void writePointer(g3d::NameReloc& reloc, u32 string_location) {
    const u32 pointer_location = reloc.offset_of_pointer_in_struct;
    const u32 structure_location = reloc.offset_of_delta_reference;
    rsl::store<s32>(string_location - structure_location, mBuf,
                    pointer_location);
  }
  u32 writeName(std::string_view name) {
    const u32 sz = name.size();
    mBuf.push_back((sz & 0xff000000) >> 24);
    mBuf.push_back((sz & 0x00ff0000) >> 16);
    mBuf.push_back((sz & 0x0000ff00) >> 8);
    mBuf.push_back((sz & 0x000000ff) >> 0);
    const u32 string_location = mBuf.size();
    mBuf.insert(mBuf.end(), name.begin(), name.end());
    mBuf.push_back(0);
    return string_location;
  }
  std::vector<u8>& mBuf;
};

std::vector<u8> WriteTEX0(const g3d::TextureData& tex) {
  auto memory_request = g3d::CalcTextureBlockData(tex);
  std::vector<u8> buffer(memory_request.size);

  g3d::NameReloc reloc;
  g3d::WriteTexture(buffer, tex, 0, reloc);
  SimpleRelocApplier applier(buffer);
  applier.apply(reloc, 0 /* structure offset */);

  return buffer;
}

Result<void> WriteTEX0ToFile(const g3d::TextureData& tex, std::string path) {
  auto buf = WriteTEX0(tex);
  TRY(rsl::WriteFile(buf, path));
  return {};
}

} // namespace librii::g3d
