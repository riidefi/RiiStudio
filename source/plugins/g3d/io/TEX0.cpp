#include <core/common.h>
#include <plugins/g3d/collection.hpp>
#include <plugins/g3d/util/NameTable.hpp>

namespace riistudio::g3d {

void writeTexture(const Texture& data, oishii::Writer& writer,
                  NameTable& names) {
  const auto start = writer.tell();

  writer.write<u32>('TEX0');
  writer.write<u32>(64 + data.getEncodedSize(true));
  writer.write<u32>(3);      // revision
  writer.write<s32>(-start); // brres offset
  writer.write<s32>(64);     // texture offset
  writeNameForward(names, writer, start, data.name);
  writer.write<u32>(0); // flag, ci
  writer.write<u16>(data.width);
  writer.write<u16>(data.height);
  writer.write<u32>(static_cast<u32>(data.format));
  writer.write<u32>(data.mipLevel);
  writer.write<f32>(data.custom_lod ? data.minLod : 0.0f);
  writer.write<f32>(data.custom_lod ? data.maxLod
                                    : static_cast<float>(data.mipLevel - 1));
  writer.write<u32>(0); // src path
  writer.write<u32>(0); // user data
  writer.alignTo(32);   // Assumes already 32b aligned
  // TODO
  for (const u8* pData = data.getData();
       pData < data.getData() + data.getEncodedSize(true); ++pData)
    writer.write<u8>(*pData);
}
void readTexture(Texture& data, oishii::BinaryReader& reader) {
  const auto start = reader.tell();

  reader.expectMagic<'TEX0', false>();
  reader.read<u32>(); // size
  const u32 revision = reader.read<u32>();
  (void)revision;
  assert(revision == 1 || revision == 3);
  reader.read<s32>(); // BRRES offset
  const s32 ofsTex = reader.read<s32>();
  data.name = readName(reader, start);
  // const u32 flag =
  reader.read<u32>(); // TODO: Paletted textures
  data.width = reader.read<u16>();
  data.height = reader.read<u16>();
  data.format = reader.read<u32>();
  data.mipLevel = reader.read<u32>();
  data.minLod = reader.read<f32>();
  data.maxLod = reader.read<f32>();
  // data.custom_lod = data.minLod != 0.0f ||
  //                   data.maxLod != static_cast<float>(data.mipLevel - 1);
  data.sourcePath = readName(reader, start);
  // Skip user data
  reader.seekSet(start + ofsTex);
  data.resizeData();
  assert(reader.tell() + data.getEncodedSize(true) < reader.endpos());
  memcpy(data.data.data(), reader.getStreamStart() + reader.tell(),
         data.getEncodedSize(true));
  reader.skip(data.getEncodedSize(true));
}

} // namespace riistudio::g3d
