#include "JPABaseShapeBlock.hpp"

#include "librii/jparticle/utils/JPAUtils.hpp"
#include "oishii/writer/binary_writer.hxx"

namespace librii::jpa {

jeff_jpa::JPABaseShapeBlock
To_JEFF_JPABaseShapeBlock(const JPABaseShapeBlock& b) {


  u8 texAnmCalcFlags = (b.isGlblTexAnm << 1) | (b.texIdxLoopOfstMask == 0xFFFF ? 1 : 0);
  u8 colorAnmCalcFlags =
      (b.isGlblTexAnm << 1) | (b.colorLoopOfstMask == 0xFFFF ? 1 : 0);

  u8 blendMode = b.blendModeFlags & 0x3;
  u8 blendSrcFactor = (b.blendModeFlags >> 2) & 0x7;
  // u8 logicOp = (b.blendModeFlags >> 4) & 0x5;
  u8 blendDstFactor = (b.blendModeFlags >> 6) & 0x7;

  u8 alphaCmp0 = b.alphaCompareFlags & 0x7;
  u8 alphaOp = (b.alphaCompareFlags >> 3) & 0x3;
  u8 alphaCmp1 = (b.alphaCompareFlags >> 5) & 0x7;

  // 0x3E is ZCompLoc
  u8 zTest = b.zModeFlags & 0x1;
  u8 zCompare = (b.zModeFlags >> 1) & 0x7;
  u8 zWrite = (b.zModeFlags >> 4) & 0xF;

  u8 flags =
      (b.isDrawFwdAhead << 1) & (b.isDrawPrntAhead);

  u8 colorPrmAnimFlags = 0x1;
  u8 colorEnvAnimFlags = 0x1;


  if (b.colorPrmAnimData.size() > 1) {
    colorPrmAnimFlags = 0x7;
  }

    if (b.colorEnvAnimData.size() > 1) {
    colorEnvAnimFlags = 0x7;
  }

  return jeff_jpa::JPABaseShapeBlock{
      .texIdxAnimDataOffs = 0, .colorPrmAnimDataOffs = 0,
      .colorEnvAnimDataOffs = 0, .baseSizeX = b.baseSize.x,
      .baseSizeY = b.baseSize.y,
      .anmRndm = b.anmRndm,
      .texAnmCalcFlags = texAnmCalcFlags,
      .colorAnmCalcFlags = colorAnmCalcFlags,
      .shapeType = static_cast<u8>(b.shapeType),
      .dirType = static_cast<u8>(b.dirType),
      .rotType = static_cast<u8>(b.rotType), .colorInSelect = b.colorInSelect,
      .blendMode = blendMode,
      .blendSrcFactor = blendSrcFactor,
      .blendDstFactor = blendDstFactor,
      .logicOp = 0,
      .alphaCmp0 = alphaCmp0,
      .alphaRef0 = b.alphaRef0,
      .alphaOp = alphaOp,
      .alphaCmp1 = alphaCmp1,
      .alphaRef1 = b.alphaRef1,
      .zCompLoc = 0,
      .zTest = zTest,
      .zCompare = zCompare,
      .zWrite = zWrite,
      .isEnableProjection = b.isEnableProjection,
      .flags = flags,
      .texAnimFlags = 0, .texCalcIdxType = static_cast<u8>(b.texCalcIdxType),
      .texIdxAnimDataCount = 0, .texIdx = 0,
      .colorAnimMaxFrm = b.colorAnimMaxFrm,
      .colorCalcIdxType = static_cast<u8>(b.colorCalcIdxType),
      .colorPrmAnimFlags = colorPrmAnimFlags,
      .colorEnvAnimFlags = colorEnvAnimFlags,
      .colorPrmAnimDataCount = static_cast<u8>(b.colorPrmAnimData.size()),
      .colorEnvAnimDataCount = static_cast<u8>(b.colorEnvAnimData.size()),
      .colorPrm = gx::Color(b.colorPrm),
      .colorEnv = gx::Color(b.colorEnv),

      .texInitTransX = JPAConvertFloatToFixed(b.texInitTransX / 10.0f),
      .texInitTransY = JPAConvertFloatToFixed(b.texInitTransY / 10.0f),
      .texInitScaleX = JPAConvertFloatToFixed(b.texInitScaleX / 10.0f),
      .texInitScaleY = JPAConvertFloatToFixed(b.texInitScaleY / 10.0f),

      .tilingS = JPAConvertFloatToFixed(b.tilingS / 10.0f),
      .tilingT = JPAConvertFloatToFixed(b.tilingT / 10.0f),
      .texIncTransX = JPAConvertFloatToFixed(b.texIncTransX),
      .texIncTransY = JPAConvertFloatToFixed(b.texIncTransY),

      .texIncScaleX = JPAConvertFloatToFixed(b.texIncScaleX / 0.1f),
      .texIncScaleY = JPAConvertFloatToFixed(b.texIncScaleY / 0.1f),
      .texIncRot = JPAConvertFloatToFixed(b.texIncRot),
      .isEnableTexScrollAnm = JPAConvertFloatToFixed(b.isEnableTexScrollAnm),
  };

}

Result<void> WriteJEFF_JPABaseShapeBlock(oishii::Writer& writer,
                                         jeff_jpa::JPABaseShapeBlock& b) {

  writer.write(b.magic);
  writer.write(b.size);
  writer.write(b.padding);
  writer.write(b._0C);
  writer.write(b._10);
  writer.write(b.texIdxAnimDataOffs);
  writer.write(b.colorPrmAnimDataOffs);
  writer.write(b.colorEnvAnimDataOffs);
  writer.write(b.baseSizeX);
  writer.write(b.baseSizeY);
  writer.write(b.anmRndm);
  writer.write(b.texAnmCalcFlags);
  writer.write(b.colorAnmCalcFlags);
  writer.write(b.shapeType);
  writer.write(b.dirType);
  writer.write(b.rotType);

  writer.write(b._27);
  writer.write(b._28);
  writer.write(b._2C);

  writer.write(b.colorInSelect);
  writer.write(b._31);
  writer.write(b._32);
  writer.write(b._34);

  writer.write(b.blendMode);
  writer.write(b.blendSrcFactor);
  writer.write(b.blendDstFactor);
  writer.write(b.logicOp);

  writer.write(b.alphaCmp0);
  writer.write(b.alphaRef0);
  writer.write(b.alphaOp);
  writer.write(b.alphaCmp1);
  writer.write(b.alphaRef1);

  writer.write(b.zCompLoc);
  writer.write(b.zTest);
  writer.write(b.zCompare);
  writer.write(b.zWrite);

  writer.write(b._42);
  writer.write(b.isEnableProjection);
  writer.write(b.flags);

  writer.write(b._45);
  writer.write(b._46);
  writer.write(b._48);

  writer.write(b.texAnimFlags);
  writer.write(b.texCalcIdxType);
  writer.write(b.texIdxAnimDataCount);
  writer.write(b.texIdx);

  writer.write(b._50);
  writer.write(b._54);
  writer.write(b._58);

  writer.write(b.colorAnimMaxFrm);
  writer.write(b.colorCalcIdxType);

  writer.write(b._5F);

  writer.write(b.colorPrmAnimFlags);
  writer.write(b.colorEnvAnimFlags);

  writer.write(b.colorPrmAnimDataCount);
  writer.write(b.colorEnvAnimDataCount);

  writer.write(b.colorPrm);
  writer.write(b.colorEnv);
  writer.write(b._6C);
  writer.write(b._70);
  writer.write(b._74);
  writer.write(b._78);
  writer.write(b._7C);

  writer.write(b.texInitTransX);
  writer.write(b.texInitTransY);
  writer.write(b.texInitScaleX);
  writer.write(b.texInitScaleY);

  writer.write(b.tilingS);
  writer.write(b.tilingT);
  writer.write(b.texIncTransX);
  writer.write(b.texIncTransY);

  writer.write(b.texIncScaleX);
  writer.write(b.texIncScaleY);
  writer.write(b.texIncRot);
  writer.write(b.isEnableTexScrollAnm);
  writer.write(b._98);
  writer.write(b._9C);

  return {};
}

}
