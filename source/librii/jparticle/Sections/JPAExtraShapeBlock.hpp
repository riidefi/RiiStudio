

namespace librii::jpa {

enum class CalcScaleAnmType {
  Normal = 0x00,
  Repeat = 0x01,
  Reverse = 0x02,
};

enum class CalcAlphaWaveType{
  None,
  NrmSin,
  AddSin,
  MultSin,
};

namespace jeff_jpa {
struct JPAExtraShapeBlock {
  u32 magic = 'ESP1';
  u32 size = 0x80;
  u32 padding = 0;
  u32 _0C = 0x14345C00;
  u32 _10 = 0;
  u16 alphaInTiming;
  u16 alphaOutTiming;
  u16 alphaInValue;
  u16 alphaBaseValue;
  u16 alphaOutValue;
  u8 alphaAnmFlags;
  u8 alphaWaveTypeFlag;
  u16 alphaWaveParam1;
  u16 alphaWaveParam2;
  u16 alphaWaveParam3;
  u16 alphaWaveRandom;
  u32 _28 = 0;
  u32 _2C = 0;
  u32 _30 = 0;
  u16 scaleOutRandom;
  u16 scaleInTiming;
  u16 scaleOutTiming;
  u16 scaleInValueY;
  u16 _3C = 0x0CCC;
  u16 scaleOutValueY;
  u8 pivotY;
  u8 anmTypeY;
  u16 scaleAnmMaxFrameY;
  u16 scaleInValueX;
  u16 _46 = 0x0CCC;
  u16 scaleOutValueX;
  u8 pivotX;
  u8 anmTypeX;
  u16 scaleAnmMaxFrameX;
  u8 scaleAnmFlags;
  u8 _4F = 0;
  u32 _50 = 0;
  u32 _54 = 0;
  u16 _58 = 0;
  u16 rotateAngle;
  u16 rotateSpeed;
  u16 rotateAngleRandom;
  u16 rotateSpeedRandom;
  u16 rotateDirection;
  u8 isEnableRotate;
  u8 _65 = 0;
  u16 _66 = 0;
  u32 _68 = 0;
  u32 _6c = 0;
  u32 _70 = 0;
  u32 _74 = 0;
  u32 _78 = 0;
  u32 _7C = 0;
};
}

struct JPAExtraShapeBlock {
  bool isEnableScale;
  bool isDiffXY;
  bool isEnableScaleBySpeedX;
  bool isEnableScaleBySpeedY;
  CalcScaleAnmType scaleAnmTypeX;
  CalcScaleAnmType scaleAnmTypeY;
  bool isEnableRotate;
  bool isEnableAlpha;
  CalcAlphaWaveType alphaWaveType;
  u8 pivotX;
  u8 pivotY;
  f32 scaleInTiming;
  f32 scaleOutTiming;
  f32 scaleInValueX;
  f32 scaleOutValueX;
  f32 scaleInValueY;
  f32 scaleOutValueY;
  f32 scaleOutRandom;
  u16 scaleAnmMaxFrameX;
  u16 scaleAnmMaxFrameY;
  f32 scaleIncreaseRateX;
  f32 scaleIncreaseRateY;
  f32 scaleDecreaseRateX;
  f32 scaleDecreaseRateY;
  f32 alphaInTiming;
  f32 alphaOutTiming;
  f32 alphaInValue;
  f32 alphaBaseValue;
  f32 alphaOutValue;
  f32 alphaIncreaseRate;
  f32 alphaDecreaseRate;
  f32 alphaWaveParam1;
  f32 alphaWaveParam2;
  f32 alphaWaveParam3;
  f32 alphaWaveRandom;
  f32 rotateAngle;
  f32 rotateAngleRandom;
  f32 rotateSpeed;
  f32 rotateSpeedRandom;
  f32 rotateDirection;
};

jeff_jpa::JPAExtraShapeBlock
To_JEFF_JPAExtraShapeBlock(const JPAExtraShapeBlock& b);

}
