#include "Blight.hpp"
#include <rsl/SafeReader.hpp>

namespace librii::egg {

// Bring into namespace
#include <core/util/glm_io.hpp>

static Result<gx::Color> readColor(rsl::SafeReader& reader) {
  auto r = TRY(reader.U8());
  auto g = TRY(reader.U8());
  auto b = TRY(reader.U8());
  auto a = TRY(reader.U8());
  return gx::Color(r, g, b, a);
}

inline void operator>>(const librii::gx::Color& out, oishii::Writer& writer) {
  librii::gx::writeColorComponents(writer, out,
                                   librii::gx::VertexBufferType::Color::rgba8);
}

Result<void> Blight::read(oishii::BinaryReader& unsafeReader) {
  unsafeReader.setEndian(std::endian::big);
  rsl::SafeReader reader(unsafeReader);

  TRY(reader.Magic("LGHT"));
  TRY(reader.U32());          // file size 0x5A8
  version = TRY(reader.U8()); // 2
  EXPECT(version == 2);       // The runtime supports older versions; we do not
  for (auto& e : reserved) {
    e = TRY(reader.U8());
  }
  auto lightObjectCount = TRY(reader.U16());
  auto ambientObjectCount = TRY(reader.U16());
  backColor = TRY(readColor(reader));
  unsafeReader.skip(16);

  reader.seekSet(0x28);
  for (u16 i = 0; i < lightObjectCount; i++) {
    LightObject& obj = lightObjects.emplace_back();

    reader.Magic("LOBJ");
    auto size = TRY(reader.U32());
    obj.version = TRY(reader.U8());
    unsafeReader.skip(3);
    TRY(reader.U32());
    obj.spotFunction = TRY(reader.Enum8<gx::SpotFn>());
    obj.distAttnFunction = TRY(reader.Enum8<gx::DistAttnFn>());
    obj.coordSpace = TRY(reader.Enum8<CoordinateSpace>());
    obj.lightType = TRY(reader.Enum8<LightType>());
    obj.ambientLightIndex = TRY(reader.U16());
    obj.flags = TRY(reader.U16());
    obj.position << unsafeReader;
    obj.aim << unsafeReader;
    obj.intensity = TRY(reader.F32());
    obj.color = TRY(readColor(reader));
    obj.specularColor = TRY(readColor(reader));
    obj.spotCutoffAngle = TRY(reader.F32());
    obj.refDist = TRY(reader.F32());
    obj.refBright = TRY(reader.F32());
    TRY(reader.U32());
    obj.snapTargetIndex = TRY(reader.U16());
    TRY(reader.U16());
  }
  for (u16 i = 0; i < ambientObjectCount; i++) {
    AmbientObject& obj = ambientObjects.emplace_back();
    obj.color = TRY(readColor(reader));
    for (auto& e : obj.reserved)
      e = TRY(reader.U8());
  }
  return {};
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
    writer.write<f32>(obj.refBright);
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
