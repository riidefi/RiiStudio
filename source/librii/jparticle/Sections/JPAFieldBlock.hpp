#pragma once

#include "glm/vec3.hpp"
#include <core/common.h>

namespace librii::jpa {

enum class FieldType {
  Gravity = 0x00,
  Air = 0x01,
  Magnet = 0x02,
  Newton = 0x03,
  Vortex = 0x04,
  Random = 0x05,
  Drag = 0x06,
  Convection = 0x07,
  Spin = 0x08,
};

enum class FieldAddType {
  FieldAccel = 0x00,
  BaseVelocity = 0x01,
  FieldVelocity = 0x02,
};

enum class FieldStatusFlag {
  NoInheritRotate = 0x02,
  AirDrag = 0x04,

  FadeUseEnTime = 0x08,
  FadeUseDisTime = 0x10,
  FadeUseFadeIn = 0x20,
  FadeUseFadeOut = 0x40,
  FadeFlagMask =
      (FadeUseEnTime | FadeUseDisTime | FadeUseFadeIn | FadeUseFadeOut),

  UseMaxDist = 0x80,
};

namespace jeff_jpa {
struct JPAFieldBlock {
  u32 magic = 'FLD1';
  u32 size = 0x60;
  u32 padding = 0;
  u8 type;
  u8 _D = 0;
  u8 addType;
  u8 cycle;
  u8 sttFlag;
  u8 _F = 0;
  u16 _10 = 0;
  f32 mag;
  f32 magRndm;
  f32 maxDist;
  f32 posX;
  f32 posY;
  f32 posZ;
  f32 dirX;
  f32 dirY;
  f32 dirZ;
  f32 param1;
  f32 param2;
  f32 param3;
  u16 fadeIn;
  u16 fadeOut;
  u16 enTime;
  u16 disTime;
  u32 _48 = 0;
  u32 _4C = 0;
  u32 _50 = 0;
  u32 _54 = 0;
  u32 _58 = 0;
};

} // namespace jeff_jpa

struct JPAFieldBlock {
  u32 sttFlag;
  FieldType type;
  FieldAddType addType;
  f32 maxDist;
  glm::vec3 pos;
  glm::vec3 dir;
  f32 fadeIn;
  f32 fadeOut;
  f32 disTime;
  f32 enTime;
  u8 cycle;
  f32 fadeInRate;
  f32 fadeOutRate;

  // Used by Gravity, Air, Magnet, Newton, Vortex, Random, Drag, Convection,
  // Spin
  f32 mag;
  // Used by Drag
  f32 magRndm;
  // Used by Newton, Air and Convection
  f32 refDistance;
  // Used by Vortex and Spin
  f32 innerSpeed;
  // Used by Vortex
  f32 outerSpeed;
};

JPAFieldBlock From_JEFF_JPAFieldBlock(const jeff_jpa::JPAFieldBlock& b);
jeff_jpa::JPAFieldBlock To_JEFF_JPAFieldBlock(const JPAFieldBlock& b);

} // namespace librii::jpa
