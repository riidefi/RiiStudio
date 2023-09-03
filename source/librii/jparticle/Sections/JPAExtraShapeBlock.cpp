#include "JPAExtraShapeBlock.hpp"

#include "librii/jparticle/utils/JPAUtils.hpp"

#include <corecrt_math_defines.h>
#include <iostream>

namespace librii::jpa {
jeff_jpa::JPAExtraShapeBlock
To_JEFF_JPAExtraShapeBlock(const JPAExtraShapeBlock& b) {


  u8 alphaAnmFlags = 0;
  alphaAnmFlags |= b.isEnableAlpha << 0;
  alphaAnmFlags |= (b.alphaWaveType != CalcAlphaWaveType::None) << 1;


  u8 scaleAnmFlags = 1;

  scaleAnmFlags |= b.isEnableScale << 1;
  scaleAnmFlags |= b.isDiffXY << 2;
  scaleAnmFlags |= (b.scaleAnmTypeY != CalcScaleAnmType::Normal) << 3;
  scaleAnmFlags |= (b.scaleAnmTypeX != CalcScaleAnmType::Normal) << 4;
  scaleAnmFlags |= b.isEnableScaleBySpeedY << 5;
  scaleAnmFlags |= b.isEnableScaleBySpeedX << 6;

  bool anmTypeX = (b.scaleAnmTypeY == CalcScaleAnmType::Reverse);
  bool anmTypeY = (b.scaleAnmTypeY == CalcScaleAnmType::Reverse);

  u8 alphaWaveTypeFlag = static_cast<u8>(b.alphaWaveType);


  return jeff_jpa::JPAExtraShapeBlock{
      .alphaInTiming = JPAConvertFloatToFixed(b.alphaInTiming),
      .alphaOutTiming = JPAConvertFloatToFixed(b.alphaOutTiming),
      .alphaInValue = JPAConvertFloatToFixed(b.alphaInValue),
      .alphaBaseValue = JPAConvertFloatToFixed(b.alphaBaseValue),
      .alphaOutValue = JPAConvertFloatToFixed(b.alphaOutValue),
      .alphaAnmFlags = alphaAnmFlags,
      .alphaWaveTypeFlag = alphaWaveTypeFlag,
      .alphaWaveParam1 = JPAConvertFloatToFixed(b.alphaWaveParam1),
      .alphaWaveParam2 = JPAConvertFloatToFixed(b.alphaWaveParam2),
      .alphaWaveParam3 = JPAConvertFloatToFixed(b.alphaWaveParam3),
      .alphaWaveRandom = JPAConvertFloatToFixed(b.alphaWaveRandom),
      .scaleOutRandom = JPAConvertFloatToFixed(b.scaleOutRandom),
      .scaleInTiming = JPAConvertFloatToFixed(b.scaleInTiming),
      .scaleOutTiming = JPAConvertFloatToFixed(b.scaleOutTiming),
      .scaleInValueY = JPAConvertFloatToFixed(b.scaleInValueY),
      .scaleOutValueY = JPAConvertFloatToFixed(b.scaleOutValueY),
      .pivotY = b.pivotY,
      .anmTypeY = anmTypeY,
      .scaleAnmMaxFrameY = b.scaleAnmMaxFrameY,
      .scaleInValueX = JPAConvertFloatToFixed(b.scaleInValueX),
      .scaleOutValueX = JPAConvertFloatToFixed(b.scaleOutValueX),
      .pivotX = b.pivotX,
      .anmTypeX = anmTypeX,
      .scaleAnmMaxFrameX = b.scaleAnmMaxFrameX,
      .scaleAnmFlags = scaleAnmFlags,
      .rotateAngle = JPAConvertFloatToFixed(b.rotateAngle / (M_PI * 2.0f)),
      .rotateSpeed = JPAConvertFloatToFixed(b.rotateSpeed / (M_PI * 2.0f)),
      .rotateAngleRandom =
          JPAConvertFloatToFixed(b.rotateAngleRandom / (M_PI * 2.0f)),
      .rotateSpeedRandom = JPAConvertFloatToFixed(b.rotateSpeedRandom),
      .rotateDirection = JPAConvertFloatToFixed(b.rotateDirection),
      .isEnableRotate = b.isEnableRotate
  };

}
} // namespace librii::jpa
