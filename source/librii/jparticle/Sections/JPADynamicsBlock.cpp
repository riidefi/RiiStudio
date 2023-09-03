#include "JPADynamicsBlock.hpp"

#include "glm/gtc/type_ptr.hpp"
#include "librii/jparticle/utils/JPAUtils.hpp"
#include "rsl/SafeReader.hpp"

namespace librii::jpa {

JPADynamicsBlock
From_JEFF_JPADynamicsBlock(const jeff_jpa::JPADynamicsBlock& b) {
  return JPADynamicsBlock{
      .emitFlags = b.emitFlags,
      .volumeType = static_cast<VolumeType>(b.volumeType),
      .emitterScl = glm::vec3{b.emitterSclX, b.emitterSclY, b.emitterSclZ},
      .emitterRot = glm::vec3{JPAConvertFixToFloat(b.emitterRotX),
                              JPAConvertFixToFloat(b.emitterRotY),
                              JPAConvertFixToFloat(b.emitterRotZ)},
      .emitterTrs = glm::vec3{b.emitterTrsX, b.emitterTrsY, b.emitterTrsZ},
      .emitterDir = glm::vec3{JPAConvertFixToFloat(b.emitterDirX),
                              JPAConvertFixToFloat(b.emitterDirY),
                              JPAConvertFixToFloat(b.emitterDirZ)},
      .initialVelOmni = b.initialVelOmni,
      .initialVelAxis = b.initialVelAxis,
      .initialVelRndm = b.initialVelRndm,
      .initialVelDir = b.initialVelDir,
      .spread = JPAConvertFixToFloat(b.spread),
      .initialVelRatio = JPAConvertFixToFloat(b.initialVelRatio),
      .rate = b.rate,
      .rateRndm = JPAConvertFixToFloat(b.rateRndm),
      .lifeTimeRndm = JPAConvertFixToFloat(b.lifeTimeRndm),
      .volumeSweep = JPAConvertFixToFloat(b.volumeSweep),
      .volumeMinRad = JPAConvertFixToFloat(b.volumeMinRad),
      .airResist = JPAConvertFixToFloat(b.airResist),
      .airResistRndm = JPAConvertFixToFloat(b.airResistRndm),
      .moment = JPAConvertFixToFloat(b.moment),
      .momentRndm = JPAConvertFixToFloat(b.momentRndm),
      .accel = b.accel,
      .accelRndm = JPAConvertFixToFloat(b.accelRndm),
      .maxFrame = b.maxFrame,
      .startFrame = b.startFrame,
      .lifeTime = b.lifeTime,
      .volumeSize = b.volumeSize,
      .divNumber = b.divNumber,
      .rateStep = b.rateStep,
  };
}

jeff_jpa::JPADynamicsBlock To_JEFF_JPADynamicsBlock(const JPADynamicsBlock& b,
                                                    u32 kfa1KeyTypeMask) {

  return jeff_jpa::JPADynamicsBlock{
      .emitterSclX = b.emitterScl.x,
      .emitterSclY = b.emitterScl.y,
      .emitterSclZ = b.emitterScl.z,
      .emitterTrsX = b.emitterTrs.x,
      .emitterTrsY = b.emitterTrs.y,
      .emitterTrsZ = b.emitterTrs.z,
      .emitterRotX = JPAConvertFloatToFixed(b.emitterRot.x),
      .emitterRotY = JPAConvertFloatToFixed(b.emitterRot.y),
      .emitterRotZ = JPAConvertFloatToFixed(b.emitterRot.z),
      .volumeType = static_cast<u8>(b.volumeType),
      .rateStep = b.rateStep,
      .divNumber = b.divNumber,
      .rate = b.rate,
      .rateRndm = JPAConvertFloatToFixed(b.rateRndm),
      .maxFrame = b.maxFrame,
      .startFrame = b.startFrame,
      .volumeSize = b.volumeSize,
      .volumeSweep = JPAConvertFloatToFixed(b.volumeSweep),
      .volumeMinRad = JPAConvertFloatToFixed(b.volumeMinRad),
      .lifeTime = b.lifeTime,
      .lifeTimeRndm = JPAConvertFloatToFixed(b.lifeTimeRndm),
      .moment = JPAConvertFloatToFixed(b.moment),
      .momentRndm = JPAConvertFloatToFixed(b.momentRndm),
      .initialVelRatio = JPAConvertFloatToFixed(b.initialVelRatio),
      .accelRndm = JPAConvertFloatToFixed(b.accelRndm),
      .airResist = JPAConvertFloatToFixed(b.airResist),
      .airResistRndm = JPAConvertFloatToFixed(b.airResistRndm),
      .initialVelOmni = b.initialVelOmni,
      .initialVelAxis = b.initialVelAxis,
      .initialVelRndm = b.initialVelRndm,
      .initialVelDir = b.initialVelDir,
      .accel = b.accel,
      .emitterDirX =  JPAConvertFloatToFixed(b.emitterDir.x),
      .emitterDirY = JPAConvertFloatToFixed(b.emitterDir.y),
      .emitterDirZ = JPAConvertFloatToFixed(b.emitterDir.z),
      .spread = JPAConvertFloatToFixed(b.spread),
      .emitFlags = b.emitFlags,
      .kfa1KeyTypeMask = kfa1KeyTypeMask
  };
}
} // namespace librii::jpa
