#pragma once

#include "glm/vec3.hpp"
#include <core/common.h>

namespace librii::jpa {

enum class VolumeType {
  Cube = 0x00,
  Sphere = 0x01,
  Cylinder = 0x02,
  Torus = 0x03,
  Point = 0x04,
  Circle = 0x05,
  Line = 0x06,
};

namespace jeff_jpa {
struct JPADynamicsBlock {
  u32 magic = 'BEM1';
  u32 size = 0xA0;
  u32 padding = 0;
  f32 emitterSclX;
  f32 emitterSclY;
  f32 emitterSclZ;
  f32 emitterTrsX;
  f32 emitterTrsY;
  f32 emitterTrsZ;
  u16 emitterRotX;
  u16 emitterRotY;
  u16 emitterRotZ;
  u8 volumeType;
  u8 rateStep;
  u8 _2C = 0;
  u8 _2D = 0;
  u16 divNumber;
  f32 rate;
  u16 rateRndm;
  u16 maxFrame;
  u16 startFrame;
  u16 volumeSize;
  u16 volumeSweep;
  u16 volumeMinRad;
  u16 lifeTime;
  u16 lifeTimeRndm;
  u16 moment;
  u16 momentRndm;
  u16 initialVelRatio;
  u16 accelRndm;
  u16 airResist;
  u16 airResistRndm;
  f32 initialVelOmni;
  f32 initialVelAxis;
  f32 initialVelRndm;
  f32 initialVelDir;
  f32 accel;
  u16 emitterDirX;
  u16 emitterDirY;
  u16 emitterDirZ;
  u16 spread;
  u32 emitFlags;
  u32 kfa1KeyTypeMask;
  u32 _74 = 0;
  u32 _78 = 0;
  u32 _7C = 0;
  u32 _80 = 0;
  u32 _84 = 0;
  u32 _88 = 0;
  u32 _8C = 0;
  u32 _90 = 0;
  u32 _94 = 0;
  u32 _98 = 0;
  u32 _9C = 0;
};
} // namespace jeff_jpa

struct JPADynamicsBlock {
  u32 emitFlags;
  VolumeType volumeType;
  glm::vec3 emitterScl;
  glm::vec3 emitterRot;
  glm::vec3 emitterTrs;
  glm::vec3 emitterDir;
  f32 initialVelOmni;
  f32 initialVelAxis;
  f32 initialVelRndm;
  f32 initialVelDir;
  f32 spread;
  f32 initialVelRatio;
  f32 rate;
  f32 rateRndm;
  f32 lifeTimeRndm;
  f32 volumeSweep;
  f32 volumeMinRad;
  f32 airResist;
  f32 airResistRndm;
  f32 moment;
  f32 momentRndm;
  f32 accel;
  f32 accelRndm;
  u16 maxFrame;
  u16 startFrame;
  u16 lifeTime;
  u16 volumeSize;
  u16 divNumber;
  u8 rateStep;
};

JPADynamicsBlock
From_JEFF_JPADynamicsBlock(const jeff_jpa::JPADynamicsBlock& b);
jeff_jpa::JPADynamicsBlock To_JEFF_JPADynamicsBlock(const JPADynamicsBlock& b,
                                                    u32 kfa1KeyTypeMask);

} // namespace librii::jpa
