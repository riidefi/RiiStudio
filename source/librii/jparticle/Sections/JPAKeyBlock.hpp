#pragma once

#include <core/common.h>
#include <vector>

namespace librii::jpa {

enum class JPAKeyType {
  None = -1,
  Rate = 0x00,
  VolumeSize = 0x01,
  VolumeSweep = 0x02,
  VolumeMinRad = 0x03,
  LifeTime = 0x04,
  Moment = 0x05,
  InitialVelOmni = 0x06,
  InitialVelAxis = 0x07,
  InitialVelDir = 0x08,
  Spread = 0x09,
  Scale = 0x0A,
};

struct JPAKeyBlock {
  JPAKeyType keyType;
  std::vector<f32> keyValues;
  bool isLoopEnable;
};

void To_JEFF_JPAKeyBlock(const JPAKeyBlock& b);

} // namespace librii::jpa
