#include "Blight.hpp"

namespace librii::egg {

// Bring into namespace
#include <core/util/glm_io.hpp>

inline void operator<<(librii::gx::Color& out, oishii::BinaryReader& reader) {
  out = librii::gx::readColorComponents(
      reader, librii::gx::VertexBufferType::Color::rgba8);
}

inline void operator>>(const librii::gx::Color& out, oishii::Writer& writer) {
  librii::gx::writeColorComponents(writer, out,
                                   librii::gx::VertexBufferType::Color::rgba8);
}

void Blight::read(oishii::BinaryReader& reader) {
  reader.setEndian(std::endian::big);

  reader.expectMagic<'LGHT', false>();
  reader.read<u32>();          // file size 0x5A8
  version = reader.read<u8>(); // 2
  assert(version == 2);        // The runtime supports older versions; we do not
  for (auto& e : reserved) {
    e = reader.read<u8>();
  }
  auto lightObjectCount = reader.read<u16>();
  auto ambientObjectCount = reader.read<u16>();
  backColor << reader;
  reader.skip(16);

  reader.seekSet(0x28);
  for (u16 i = 0; i < lightObjectCount; i++) {
    LightObject& obj = lightObjects.emplace_back();

    reader.expectMagic<'LOBJ'>();
    auto size = reader.read<u32>();
    obj.version = reader.read<u8>();
    reader.skip(3);
    reader.read<u32>();
    obj.spotFunction = (gx::SpotFn)reader.read<u8>();
    obj.distAttnFunction = (gx::DistAttnFn)reader.read<u8>();
    obj.coordSpace = (CoordinateSpace)reader.read<u8>();
    obj.lightType = (LightType)reader.read<u8>();
    obj.ambientLightIndex = reader.read<u16>();
    obj.flags = reader.read<u16>();
    obj.position << reader;
    obj.aim << reader;
    obj.intensity = reader.read<f32>();
    obj.color << reader;
    obj.specularColor << reader;
    obj.spotCutoffAngle = reader.read<f32>();
    obj.refDist = reader.read<f32>();
    obj.refBrightness = reader.read<f32>();
    reader.read<u32>();
    obj.snapTargetIndex = reader.read<u16>();
    reader.read<u16>();
  }
  for (u16 i = 0; i < ambientObjectCount; i++) {
    AmbientObject& obj = ambientObjects.emplace_back();
    obj.color << reader;
    for (auto& e : obj.reserved)
      e = reader.read<u8>();
  }
}

void Blight::write(oishii::Writer& writer) {
  assert(version == 2);
  writer.setEndian(std::endian::big);
  writer.write<u32>('LGHT');
  writer.write<u32>(0x28 + 0x50 * lightObjects.size() +
                    8 * ambientObjects.size());
  writer.write<u8>(version);
  for (auto e : reserved)
    writer.write<u8>(e);
  writer.write<u16>(static_cast<u16>(lightObjects.size()));
  writer.write<u16>(static_cast<u16>(ambientObjects.size()));
  backColor >> writer;
  writer.skip(16);

  writer.seekSet(0x28);
  for (const auto& obj : lightObjects) {
    writer.write<u32>('LOBJ');
    writer.write<u32>(0x50);
    writer.write<u8>(obj.version);
    writer.skip(3);
    writer.write<u32>(0);
    writer.write<u8>(static_cast<u8>(obj.spotFunction));
    writer.write<u8>(static_cast<u8>(obj.distAttnFunction));
    writer.write<u8>(static_cast<u8>(obj.coordSpace));
    writer.write<u8>(static_cast<u8>(obj.lightType));
    writer.write<u16>(obj.ambientLightIndex);
    writer.write<u16>(obj.flags);
    obj.position >> writer;
    obj.aim >> writer;
    writer.write<f32>(obj.intensity);
    obj.color >> writer;
    obj.specularColor >> writer;
    writer.write<f32>(obj.spotCutoffAngle);
    writer.write<f32>(obj.refDist);
    writer.write<f32>(obj.refBrightness);
    writer.write<u32>(0);
    writer.write<u16>(obj.snapTargetIndex);
    writer.write<u16>(0);
  }

  for (const auto& obj : ambientObjects) {
    obj.color >> writer;
    for (auto& e : obj.reserved)
      writer.write<u8>(e);
  }
}

} // namespace librii::egg
