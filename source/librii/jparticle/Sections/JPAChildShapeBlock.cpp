#pragma once

#include "librii/gx/Color.hpp"
#include "JPAChildShapeBlock.hpp"

#include "librii/jparticle/utils/JPAUtils.hpp"
#include "oishii/writer/binary_writer.hxx"

namespace librii::jpa {

jeff_jpa::JPAChildShapeBlock
To_JEFF_JPAChildShapeBlock(const JPAChildShapeBlock& b,
                           bool isEnableDrawParent) {


  u8 flags = (b.isInheritedScale << 0) |  (b.isInheritedAlpha << 1) | (b.isInheritedRGB << 2);

  return jeff_jpa::JPAChildShapeBlock{
      .shapeType = static_cast<u8>(b.shapeType),
      .dirType = static_cast<u8>(b.dirType),
      .rotType = static_cast<u8>(b.rotType), .life = b.life, .rate = b.rate,
      .timing = JPAConvertFloatToFixed(b.timing), .step = static_cast<u8>(b.step),
      .posRndm = b.posRndm, .baseVel = b.baseVel,
      .velInfRate = JPAConvertFloatToFixed(b.velInfRate),
      .baseVelRndm = JPAConvertFloatToFixed(b.baseVelRndm),
      .gravity = JPAConvertFloatToFixed(b.gravity),
      .isEnableField = b.isEnableField,
      .isEnableDrawParent = !isEnableDrawParent,
      .isEnableScaleOut = b.isEnableScaleOut,
      .isEnableAlphaOut = b.isEnableAlphaOut,
      .texIdx = b.texIdx,
      .inheritScale = JPAConvertFloatToFixed(b.inheritScale),
      .inheritAlpha = JPAConvertFloatToFixed(b.inheritAlpha),
      .globalScale2DX = b.globalScale2D.x, .globalScale2DY = b.globalScale2D.y,
      .rotateSpeed = JPAConvertFloatToFixed(b.rotateSpeed),
	 .isEnableRotate = b.isEnableRotate,
      .flags = flags,
      .colorPrm = gx::Color(b.colorPrm),
      .colorEnv = gx::Color(b.colorEnv),
      .inheritRGB = JPAConvertFloatToFixed(b.inheritRGB)

  };

}

} // namespace librii::jpa
