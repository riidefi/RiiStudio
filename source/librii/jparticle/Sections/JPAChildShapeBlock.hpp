#pragma once

#include "librii/gx/Color.hpp"
#include "JPABaseShapeBlock.hpp"
#include "oishii/writer/binary_writer.hxx"

namespace librii::jpa {

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
} // namespace librii::jpa
