#pragma once

#include "librii/gx/Color.hpp"
#include "JPABaseShapeBlock.hpp"
#include "oishii/writer/binary_writer.hxx"

namespace librii::jpa {

namespace jeff_jpa {
struct JPAChildShapeBlock {
  u32 magic = 'SSP1';
  u32 size = 0x70;
  u32 padding = 0;
  u32 _0C = 0x14345C00;
  u8 shapeType;
  u8 dirType;
  u8 rotType;
  u8 _13;
  u16 life;
  u16 rate; // 0x16
  u16 timing; // 0x18
  u8 step; // 0x1A
  u8 _1B;    // 0x1A
  u32 _1C;
  u32 _20;
  u32 _24;
  f32 posRndm;
  f32 baseVel;
  u16 velInfRate;
  u16 baseVelRndm;
  u16 gravity;
  u8 isEnableField;
  u8 _37;
  u32 _38;
  u32 _3C;
  u32 _40;
  u8 isEnableDrawParent;
  u8 isEnableScaleOut;
  u8 isEnableAlphaOut;
  u8 texIdx;
  u16 inheritScale;
  u16 inheritAlpha;
  f32 globalScale2DX;
  f32 globalScale2DY;
  u16 rotateSpeed;
  u8 isEnableRotate;
  u8 flags;
  u32 colorPrm;
  u32 colorEnv;
  u16 inheritRGB;
  u16 _62;
  u32 _64;
  u32 _68;
  u32 _6C;
};
}


struct JPAChildShapeBlock {
  bool isInheritedScale;
  bool isInheritedRGB;
  bool isInheritedAlpha;
  bool isEnableAlphaOut;
  bool isEnableField;
  bool isEnableRotate;
  bool isEnableScaleOut;
  ShapeType shapeType;
  DirType dirType;
  RotType rotType;
  PlaneType planeType;
  f32 posRndm;
  f32 baseVel;
  f32 baseVelRndm;
  f32 velInfRate;
  f32 gravity;
  glm::vec2 globalScale2D;
  f32 inheritScale;
  f32 inheritAlpha;
  f32 inheritRGB;
  gx::ColorF32 colorPrm;
  gx::ColorF32 colorEnv;
  f32 timing;
  u16 life;
  u16 rate;
  u16 step;
  u8 texIdx;
  f32 rotateSpeed;
};

jeff_jpa::JPAChildShapeBlock
To_JEFF_JPAChildShapeBlock(const JPAChildShapeBlock& b,
                           bool isEnableDrawParent);
} // namespace librii::jpa
