#include "LTEX.hpp"

namespace librii::egg {

Result<void> LightTexture::read(rsl::SafeReader& reader) {
  TRY(reader.Magic("LTEX"));
  auto fileSize = TRY(reader.U32());
  if (fileSize != 0x13C) {
    return std::unexpected("Unexpected LTEX entry size");
  }
  auto revision = TRY(reader.U8());
  if (revision != 0) {
    return std::unexpected(
        std::format("Expected LTEX version 0, saw version {}", revision));
  }
  reserved0 = TRY(reader.U8Buffer<7>());
  auto numSetting = TRY(reader.U16());
  reserved1 = TRY(reader.U8Buffer<2>());
  textureName = TRY(reader.CharBuffer<32>());
  baseLayer = TRY(reader.Enum8<BaseLayer>());
  reserved2 = TRY(reader.U8Buffer<3>());
  activeDrawSettings = TRY(reader.U32());

  drawSettings.resize(numSetting);
  for (auto& setting : drawSettings) {
    setting.normEffectScale = TRY(reader.F32());
    setting.pattern = TRY(reader.Enum8<cPttrn>());
    setting.stackjunk = TRY(reader.U8Buffer<3>());
  }

  return {};
}
void LightTexture::write(oishii::Writer& writer) const {
  writer.write<u32>('LTEX');
  writer.write<u32>(0x13C);
  writer.write<u8>(0);
  for (auto e : reserved0)
    writer.write<u8>(e);
  writer.write<u16>(drawSettings.size());
  for (auto e : reserved1)
    writer.write<u8>(e);
  for (auto e : textureName)
    writer.write<char>(e);
  writer.write<u8>(static_cast<u8>(baseLayer));
  for (auto e : reserved2)
    writer.write<u8>(e);
  writer.write<u32>(activeDrawSettings);

  for (const auto& setting : drawSettings) {
    writer.write<f32>(setting.normEffectScale);
    writer.write<u8>(static_cast<u8>(setting.pattern));
    for (auto e : setting.stackjunk)
      writer.write<u8>(e);
  }
}

Result<void> LightMap::read(rsl::SafeReader& reader) {
  TRY(reader.Magic("LMAP"));
  auto fileSize = TRY(reader.U32());
  auto revision = TRY(reader.U8());
  if (revision != 0) {
    return std::unexpected(
        std::format("Expected LMAP version 0, saw version {}", revision));
  }
  reserved0 = TRY(reader.U8Buffer<7>());
  auto numTex = TRY(reader.U16());
  reserved1 = TRY(reader.U8Buffer<14>());

  textures.resize(numTex);
  for (auto& tex : textures) {
    TRY(tex.read(reader));
  }

  return {};
}
void LightMap::write(oishii::Writer& writer) const {
  writer.write<u32>('LMAP');
  writer.write<u32>(fileSize());
  writer.write<u8>(0);
  for (auto e : reserved0)
    writer.write<u8>(e);
  writer.write<u16>(textures.size());
  for (auto e : reserved1)
    writer.write<u8>(e);

  for (const auto& tex : textures) {
    tex.write(writer);
  }
}

} // namespace librii::egg
