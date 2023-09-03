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

  for (u32 i = 0; i<4;i++) {
    writer.write<u32>(0);
  }

  // writer.writeN(4,0);
  if (jpac.resources[0].bem1) {
    rsl::WriteFields(writer, librii::jpa::To_JEFF_JPADynamicsBlock(
                                 *jpac.resources[0].bem1, 0));
    numberOfSections++;
  }

  for (auto& field : jpac.resources[0].fld1) {
    rsl::WriteFields(writer, librii::jpa::To_JEFF_JPAFieldBlock(*field));
    numberOfSections++;
  }


  for (auto& field : jpac.resources[0].kfa1) {
    // rsl::WriteFields(writer, librii::jpa::To_JEFF_JPAFieldBlock(*field));
    numberOfSections++;
  }


  // rsl::WriteFields(
  //     writer, librii::jpa::To_JEFF_JPABaseShapeBlock(*jpac.resources[0].bsp1));

  if (jpac.resources[0].bsp1) {
    numberOfSections++;

    // BSP1 is too big to use writeFields (maximum of 64 members ) 

    // rsl::WriteFields(writer, librii::jpa::To_JEFF_JPAExtraShapeBlock(
    //                              *jpac.resources[0].esp1));

    auto bsp = librii::jpa::To_JEFF_JPABaseShapeBlock(*jpac.resources[0].bsp1);

    librii::jpa::WriteJEFF_JPABaseShapeBlock(writer, bsp);


  }


  if (jpac.resources[0].esp1) {
    numberOfSections++;
    rsl::WriteFields(writer, librii::jpa::To_JEFF_JPAExtraShapeBlock(
                                 *jpac.resources[0].esp1));
  }
  for (auto& texture : jpac.textures) {

    numberOfSections++;
    writer.write('TEX1');

    writer.write(static_cast<u32>(texture.size())+8);

    for ( auto& textureByte : texture) {
      writer.write(textureByte);
    }
  }

  u32 filesize = writer.tell();

  writer.seek(0x8);
  writer.write(filesize);
  writer.write(numberOfSections);
}
