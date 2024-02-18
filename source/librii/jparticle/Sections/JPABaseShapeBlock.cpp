#include "JPABaseShapeBlock.hpp"

#include "librii/jparticle/utils/JPAUtils.hpp"
#include "oishii/writer/binary_writer.hxx"

namespace librii::jpa {

jeff_jpa::JPABaseShapeBlock
To_JEFF_JPABaseShapeBlock(const JPABaseShapeBlock& b) {


  u8 texAnmCalcFlags = (b.isGlblTexAnm << 1) | (b.texIdxLoopOfstMask == 0xFF ? 1 : 0);
  u8 colorAnmCalcFlags =
      (b.isGlblClrAnm << 1) | (b.colorLoopOfstMask == 0xFF ? 1 : 0);

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

  u8 texAnimFlags = 0x2;
  texAnimFlags |= b.texIdxAnimData.size() > 0;


  u16 size = 0xA0;

  u16 texIdxAnimDataOffs = b.texIdxAnimData.size() > 0 ? size : 0;
  size += sizeof(u8) * b.texIdxAnimData.size();


  u16 colorPrmAnimDataOffs = b.colorPrmAnimData.size() > 0 ? size : 0;
  // a color table entry is made up of a u16 and a rgba stored as 4 u8a
  size += (sizeof(u16) + sizeof(u32)) * b.colorPrmAnimData.size();

  u16 colorEnvAnimDataOffs = b.colorEnvAnimData.size() > 0 ? size : 0;
  size += (sizeof(u16) + sizeof(u32)) * b.colorEnvAnimData.size();

  // calculate the ceiling of size with a significance of 0x20
  size = (1 + ((size - 1) / 0x20)) * 0x20;

  return jeff_jpa::JPABaseShapeBlock{
      .size = size, 
      .texIdxAnimDataOffs = texIdxAnimDataOffs,
      .colorPrmAnimDataOffs = colorPrmAnimDataOffs,
      .colorEnvAnimDataOffs = colorEnvAnimDataOffs,
      .baseSizeX = b.baseSize.x,
      .baseSizeY = b.baseSize.y,
      .anmRndm = b.anmRndm,
      .texAnmCalcFlags = texAnmCalcFlags,
      .colorAnmCalcFlags = colorAnmCalcFlags,
      .shapeType = static_cast<u8>(b.shapeType),
      .dirType = static_cast<u8>(b.dirType),
      .rotType = static_cast<u8>(b.rotType), .colorInSelect = b.colorInSelect,
      .blendMode = static_cast<u8>(b.blendMode),
      .blendSrcFactor = static_cast<u8>(b.blendSrcFactor),
      .blendDstFactor = static_cast<u8>(b.blendDstFactor),
      .logicOp = static_cast<u8>(b.logicOp),
      .alphaCmp0 = static_cast<u8>(b.alphaCmp0),
      .alphaRef0 = b.alphaRef0,
      .alphaOp = static_cast<u8>(b.alphaOp),
      .alphaCmp1 = static_cast<u8>(b.alphaCmp1),
      .alphaRef1 = b.alphaRef1,
      .zCompLoc = 0,
      .zTest = b.zTest,
      .zCompare = static_cast<u8>(b.zCompare),
      .zWrite = b.zWrite,
      .isEnableProjection = b.isEnableProjection,
      .flags = flags,
      .texAnimFlags = texAnimFlags,
      .texCalcIdxType = static_cast<u8>(b.texCalcIdxType),
      .texIdxAnimDataCount = static_cast<u8>(b.texIdxAnimData.size()),
      .texIdx = b.texIdx,
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
      .texIdxAnimData = b.texIdxAnimData,
      .colorPrmAnimData = b.colorPrmAnimData,
      .colorEnvAnimData = b.colorEnvAnimData
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

  for (auto& entry: b.texIdxAnimData) {
    writer.write(entry);
  }
  for (auto& entry : b.colorPrmAnimData) {
    writer.write(entry.timeBegin);
    writer.write(u32(gx::Color(entry.color)));
  }
  for (auto& entry : b.colorEnvAnimData) {
    writer.write(entry.timeBegin);
    writer.write(u32(gx::Color(entry.color)));
  }
  // Now pad out to a multiple of 0x20
  for (u32 i = 0; i < (writer.tell() % 0x20); i++) {
    writer.write<u8>(0);
  }


  return {};
}

}
