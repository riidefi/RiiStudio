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
  writer.write<u32>(data.number_of_images);
  writer.write<f32>(data.custom_lod ? data.minLod : 0.0f);
  writer.write<f32>(data.custom_lod
                        ? data.maxLod
                        : static_cast<float>(data.number_of_images - 1));
  writer.write<u32>(0); // src path
  writer.write<u32>(0); // user data
  writer.alignTo(32);   // Assumes already 32b aligned
  // TODO
  const u8* begin = data.getData();
  const u8* end = begin + data.getEncodedSize(true);
  for (const u8* pData = begin; pData < end; ++pData)
    writer.write<u8>(*pData);
}

} // namespace riistudio::g3d
