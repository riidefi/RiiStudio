#pragma once


#include "librii/gx/Color.hpp"
#include "oishii/writer/binary_writer.hxx"

namespace librii::jpa {

enum class EmitFlags {
  FixedDensity = 0x01,
  FixedInterval = 0x02,
  InheritScale = 0x04,
  FollowEmitter = 0x08,
  FollowEmitterChild = 0x10,
};

enum class ShapeType {
  Point = 0x00,
  Line = 0x01,
  Billboard = 0x02,
  Direction = 0x03,
  DirectionCross = 0x04,
  Stripe = 0x05,
  StripeCross = 0x06,
  Rotation = 0x07,
  RotationCross = 0x08,
  DirBillboard = 0x09,
  YBillboard = 0x0A,
};

enum class DirType {
  Vel = 0,
  Pos = 1,
  PosInv = 2,
  EmtrDir = 3,
  PrevPctl = 4,
};

enum class RotType {
  Y = 0x00,
  X = 0x01,
  Z = 0x02,
  XYZ = 0x03,
  YJiggle = 0x04,
};

enum class PlaneType {
  XY = 0x00,
  XZ = 0x01,
  X = 0x02,
};

enum class CalcIdxType {
  Normal = 0x00,
  Repeat = 0x01,
  Reverse = 0x02,
  Merge = 0x03,
  Random = 0x04,
};

enum class IndTextureMode {
  Off = 0x00,
  Normal = 0x01,
  Sub = 0x02,
};



namespace jeff_jpa {
struct JPABaseShapeBlock {
  u32 magic = 'BSP1';
  u32 size = 0xA0;
  u32 padding = 0;
  u32 _0C = 0x18304C5C;
  u16 _10 = 0x888C;
  u16 texIdxAnimDataOffs;
  u16 colorPrmAnimDataOffs;
  u16 colorEnvAnimDataOffs;
  f32 baseSizeX;
  f32 baseSizeY;
  u16 anmRndm;
  u8 texAnmCalcFlags;
  u8 colorAnmCalcFlags;
  u8 shapeType;
  u8 dirType;
  u8 rotType;

  u8 _27 = 0;
  u32 _28 = 0;
  u32 _2C = 0;

  u8 colorInSelect;
  u8 _31 = 0;
  u16 _32 = 0;
  u8 _34 = 0;

  u8 blendMode;
  u8 blendSrcFactor;
  u8 blendDstFactor;
  u8 logicOp;

  u8 alphaCmp0;
  u8 alphaRef0;
  u8 alphaOp;
  u8 alphaCmp1;
  u8 alphaRef1;

  u8 zCompLoc;
  u8 zTest;
  u8 zCompare;
  u8 zWrite;

  u8 _42 = 0;
  u8 isEnableProjection;
  u8 flags;
  u8 _45 = 0;
  u16 _46 = 0;
  u32 _48 = 0;
  u8 texAnimFlags;
  u8 texCalcIdxType;
  u8 texIdxAnimDataCount;
  u8 texIdx;

  u32 _50 = 0;
  u32 _54 = 0;
  u32 _58 = 0;

  u16 colorAnimMaxFrm;
  u8 colorCalcIdxType;

  u8 _5F = 0;

  u8 colorPrmAnimFlags;
  u8 colorEnvAnimFlags;

  u8 colorPrmAnimDataCount;
  u8 colorEnvAnimDataCount;

  u32 colorPrm;
  u32 colorEnv;
  u32 _6C = 0;
  u32 _70 = 0;
  u32 _74 = 0;
  u32 _78 = 0;
  u32 _7C = 0;

  u16 texInitTransX;
  u16 texInitTransY;
  u16 texInitScaleX;
  u16 texInitScaleY;

  u16 tilingS;
  u16 tilingT;
  u16 texIncTransX;
  u16 texIncTransY;

  u16 texIncScaleX;
  u16 texIncScaleY;
  u16 texIncRot;
  u16 isEnableTexScrollAnm;
  u32 _98 = 0;

  u32 _9C = 0;

};
}

struct ColorTableEntry {
  ColorTableEntry(u16 timeBegin, gx::ColorF32 color)
      : timeBegin(timeBegin), color(color) {};
  u16 timeBegin;
  gx::ColorF32 color;
};


struct JPABaseShapeBlock {
  ShapeType shapeType;
  DirType dirType;
  RotType rotType;
  PlaneType planeType;
  glm::vec2 baseSize;
  f32 tilingS;
  f32 tilingT;
  bool isDrawFwdAhead;
  bool isDrawPrntAhead;
  bool isNoDrawParent;
  bool isNoDrawChild;

  // TEV/PE Settings
  u8 colorInSelect;
  u8 alphaInSelect;
  u32 blendModeFlags;
  u32 alphaCompareFlags;
  u8 alphaRef0;
  u8 alphaRef1;
  u32 zModeFlags;

  u16 anmRndm;

  // Texture Palette Animation
  bool isEnableTexture;
  bool isGlblTexAnm;
  CalcIdxType texCalcIdxType;
  u8 texIdx;
  std::vector<u8> texIdxAnimData;
  u16 texIdxLoopOfstMask;

  // Texture Coordinate Animation
  bool isEnableProjection;
  bool isEnableTexScrollAnm;
  f32 texInitTransX;
  f32 texInitTransY;
  f32 texInitScaleX;
  f32 texInitScaleY;
  f32 texInitRot;
  f32 texIncTransX;
  f32 texIncTransY;
  f32 texIncScaleX;
  f32 texIncScaleY;
  f32 texIncRot;

  // Color Animation Settings
  bool isGlblClrAnm;
  CalcIdxType colorCalcIdxType;
  gx::ColorF32 colorPrm;
  gx::ColorF32 colorEnv;
  std::vector<ColorTableEntry> colorPrmAnimData;
  std::vector<ColorTableEntry> colorEnvAnimData;
  u16 colorAnimMaxFrm;
  u16 colorLoopOfstMask;
};

jeff_jpa::JPABaseShapeBlock
To_JEFF_JPABaseShapeBlock(const JPABaseShapeBlock& b);

Result<void> WriteJEFF_JPABaseShapeBlock(oishii::Writer& writer,
                                         jeff_jpa::JPABaseShapeBlock& b);

} // namespace librii::jpa
