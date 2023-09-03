#include "JPAFieldBlock.hpp"

#include "librii/jparticle/utils/JPAUtils.hpp"

namespace librii::jpa {

JPAFieldBlock
From_JEFF_JPAFieldBlock(const jeff_jpa::JPAFieldBlock& b) {
  return JPAFieldBlock();
}

jeff_jpa::JPAFieldBlock To_JEFF_JPAFieldBlock(const JPAFieldBlock& b) {

  f32 param1 = 0;
  f32 param2 = 0;
  f32 param3 = 0;
  f32 mag = b.mag;
  f32 magRndm = b.magRndm;

  if (b.type == FieldType::Newton) {
    param1 = sqrtf(b.refDistance);
  }

  if (b.type == FieldType::Vortex) {
    mag = b.innerSpeed;
    magRndm = b.outerSpeed;
  }

  if (b.type == FieldType::Air) {
    magRndm = b.refDistance;
  }

  if (b.type == FieldType::Convection) {
    param2 = b.refDistance;
  }

  if (b.type == FieldType::Spin) {
    mag = b.innerSpeed;
  }

  return jeff_jpa::JPAFieldBlock{.type = static_cast<u8>(b.type),
                                 .addType = static_cast<u8>(b.addType),
                                 .cycle = b.cycle,
                                 .sttFlag = static_cast<u8>(b.sttFlag),
                                 .mag = mag,
                                 .magRndm = magRndm,
                                 .maxDist = b.maxDist,
                                 .posX = b.pos.x,
                                 .posY = b.pos.y,
                                 .posZ = b.pos.z,
                                 .dirX = b.dir.x,
                                 .dirY = b.dir.y,
                                 .dirZ = b.dir.z,
                                 .param1 = param1,
                                 .param2 = param2,
                                 .param3 = param3,
                                 .fadeIn = JPAConvertFloatToFixed(b.fadeIn),
                                 .fadeOut = JPAConvertFloatToFixed(b.fadeOut),
                                 .disTime = JPAConvertFloatToFixed(b.disTime),
                                 .enTime = JPAConvertFloatToFixed(b.enTime)};

}

} // namespace librii::jpa
