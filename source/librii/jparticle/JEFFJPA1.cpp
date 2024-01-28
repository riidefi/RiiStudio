#include <rsl/Reflection.hpp>

#include "JParticle.hpp"
#include "JEFFJPA1.hpp"
#include "oishii/writer/binary_writer.hxx"

void SaveAsJEFFJP(oishii::Writer& writer, const librii::jpa::JPAC& jpac) {

  u32 numberOfSections = 0;

  // Header
  writer.write('JEFF');
  writer.write('jpa1');
  writer.write<u32>(0);
  writer.write<u32>(0);

  for (u32 i = 0; i < 4; i++) {
    writer.write<u32>(0);
  }

  rsl::WriteFields(writer, librii::jpa::To_JEFF_JPADynamicsBlock(
                       jpac.resources[0].bem1, 0));
  numberOfSections++;

  for (auto& field : jpac.resources[0].fld1) {
    rsl::WriteFields(writer, librii::jpa::To_JEFF_JPAFieldBlock(field));
    numberOfSections++;
  }

  for (auto& field : jpac.resources[0].kfa1) {
    // rsl::WriteFields(writer, librii::jpa::To_JEFF_JPAKeyBlock(*field));
    numberOfSections++;
  }

  numberOfSections++;

  // BSP1 is too big to use writeFields (maximum of 64 members ) 

  auto bsp = librii::jpa::To_JEFF_JPABaseShapeBlock(jpac.resources[0].bsp1);

  librii::jpa::WriteJEFF_JPABaseShapeBlock(writer, bsp);

  if (jpac.resources[0].ssp1.has_value()) {
    numberOfSections++;
    rsl::WriteFields(writer, librii::jpa::To_JEFF_JPAChildShapeBlock(
                                 jpac.resources[0].ssp1.value(),
                                 jpac.resources[0].bsp1.isNoDrawParent));
  }

  if (jpac.resources[0].esp1.has_value()) {
    numberOfSections++;
    rsl::WriteFields(writer, librii::jpa::To_JEFF_JPAExtraShapeBlock(
                         jpac.resources[0].esp1.value()));
  }
  for (auto& texture : jpac.textures) {

    u32 sectionStart = writer.tell();
    numberOfSections++;
    writer.write('TEX1');
    auto buffer_size = librii::gx::computeImageSize(
        texture.tex.mWidth, texture.tex.mHeight, texture.tex.mFormat,
        texture.tex.mMipmapLevel);

    // Image size plus header
    writer.write(static_cast<u32>(buffer_size) + 0x40);

    // padding
    writer.write<u32>(0);

    // Texture name
    for (char& c : texture.getName()) {
      writer.write(c);
    }

    // Now pad out to a multiple of 0x20
    for (u32 i = 0; i < 0x20-((writer.tell() - sectionStart)); i++) {
      writer.write<u8>(0);
    }

    texture.tex.write(writer);

    writer.write(texture.tex.ofsTex);

    for (auto& textureByte : texture.getData()) {
      writer.write(textureByte);
    }
  }

  u32 filesize = writer.tell();

  writer.seek(0x8);
  writer.write(filesize);
  writer.write(numberOfSections);
}
