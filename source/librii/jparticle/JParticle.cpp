#include "JParticle.hpp"

#include "glm/geometric.hpp"
#include "glm/gtx/compatibility.hpp"
#include "librii/g3d/io/CommonIO.hpp"

#include "oishii/reader/binary_reader.hxx"
#include "oishii/writer/binary_writer.hxx"
#include "rsl/DumpStruct.hpp"
#include "rsl/SafeReader.hpp"

#include "utils/JPAUtils.hpp"

#include <librii/gx.h>
#include <numbers>


namespace librii::jpa {


float lint1(f32 a, f32 b, f32 f) { return (a * (1.0f - f)) + (b * f); }


gx::ColorF32 colorLerp(gx::ColorF32 a, gx::ColorF32 b, f32 t) {
  gx::ColorF32 dst = gx::ColorF32();
  dst.r = lint1(a.r, b.r, t);
  dst.g = lint1(a.g, b.g, t);
  dst.b = lint1(a.b, b.b, t);
  dst.a = lint1(a.a, b.a, t);

  return dst;
}

gx::ColorF32 colorFromRGBA8(u32 n) {
  const gx::Color dst = gx::Color(n);
  return dst;
}


Result<void> makeColorTable(oishii::BinaryReader& reader, u16 offset,
                            u16 entryCount, std::vector<ColorTableEntry>& dst) {

  assert(entryCount > 0 && duration > 0);

  for (u32 i = 0; i < entryCount; i++) {
    u32 entry0 = i;
    u32 time0 = TRY(reader.tryGetAt<u16>(offset + (entry0 * 0x06 + 0x00)));

    gx::ColorF32 scratch = colorFromRGBA8(
        TRY(reader.tryGetAt<u32, oishii::EndianSelect::Current, true>(
              offset + (entry0 * 0x06 + 0x02))));

    ColorTableEntry entry = ColorTableEntry(time0, scratch);
    dst.push_back(entry);
  }

  return {};
}

Result<void>
JPAC::load_block_data_from_file_JPAC2_10(oishii::BinaryReader& reader,
                                         s32 block_count) {
  JPAResource resource = JPAResource();

  auto blockCount = TRY(reader.tryRead<u16>());
  auto fieldBlockCount = TRY(reader.tryRead<u8>());
  auto keyBlockCount = TRY(reader.tryRead<u8>());
  auto tdb1Count = TRY(reader.tryRead<u8>());
  reader.skip(1);
  for (s32 i = 0; i < block_count; ++i) {
    auto tag_start = reader.tell();

    auto tag_name = TRY(reader.tryRead<s32>());
    auto tag_size = TRY(reader.tryRead<s32>());

    if (tag_name == 'BEM1') {
      JPADynamicsBlock block = JPADynamicsBlock();

      block.emitFlags = TRY(reader.tryGetAt<u32>(tag_start + 0x08));
      block.volumeType = static_cast<VolumeType>((block.emitFlags >> 8) & 0x07);

      // 0x0C = unk
      f32 emitterSclX = TRY(reader.tryGetAt<f32>(tag_start + 0x10));
      f32 emitterSclY = TRY(reader.tryGetAt<f32>(tag_start + 0x14));
      f32 emitterSclZ = TRY(reader.tryGetAt<f32>(tag_start + 0x18));
      block.emitterScl = glm::vec3(emitterSclX, emitterSclY, emitterSclZ);

      f32 emitterTrsX = TRY(reader.tryGetAt<f32>(tag_start + 0x1C));
      f32 emitterTrsY = TRY(reader.tryGetAt<f32>(tag_start + 0x20));
      f32 emitterTrsZ = TRY(reader.tryGetAt<f32>(tag_start + 0x24));
      block.emitterTrs = glm::vec3(emitterTrsX, emitterTrsY, emitterTrsZ);

      f32 emitterDirX = TRY(reader.tryGetAt<f32>(tag_start + 0x28));
      f32 emitterDirY = TRY(reader.tryGetAt<f32>(tag_start + 0x2C));
      f32 emitterDirZ = TRY(reader.tryGetAt<f32>(tag_start + 0x30));
      block.emitterDir =
          glm::normalize(glm::vec3(emitterDirX, emitterDirY, emitterDirZ));

      block.initialVelOmni = TRY(reader.tryGetAt<f32>(tag_start + 0x34));
      block.initialVelAxis = TRY(reader.tryGetAt<f32>(tag_start + 0x38));
      block.initialVelRndm = TRY(reader.tryGetAt<f32>(tag_start + 0x3C));
      block.initialVelDir = TRY(reader.tryGetAt<f32>(tag_start + 0x40));

      block.spread = TRY(reader.tryGetAt<f32>(tag_start + 0x44));
      block.initialVelRatio = TRY(reader.tryGetAt<f32>(tag_start + 0x48));
      block.rate = TRY(reader.tryGetAt<f32>(tag_start + 0x4C));
      block.rateRndm = TRY(reader.tryGetAt<f32>(tag_start + 0x50));
      block.lifeTimeRndm = TRY(reader.tryGetAt<f32>(tag_start + 0x54));
      block.volumeSweep = TRY(reader.tryGetAt<f32>(tag_start + 0x58));
      block.volumeMinRad = TRY(reader.tryGetAt<f32>(tag_start + 0x5C));
      block.airResist = TRY(reader.tryGetAt<f32>(tag_start + 0x60));
      block.momentRndm = TRY(reader.tryGetAt<f32>(tag_start + 0x64));

      f32 emitterRotX = (TRY(reader.tryGetAt<u16>(tag_start + 0x68))) *
                        (std::numbers::pi / 180);
      f32 emitterRotY = (TRY(reader.tryGetAt<u16>(tag_start + 0x6A))) *
                        (std::numbers::pi / 180);
      f32 emitterRotZ = (TRY(reader.tryGetAt<u16>(tag_start + 0x6C))) * (
                          std::numbers::pi / 180);
      block.emitterRot = glm::vec3(emitterRotX, emitterRotY, emitterRotZ);

      block.maxFrame = TRY(reader.tryGetAt<u16>(tag_start + 0x6E));
      block.startFrame = TRY(reader.tryGetAt<u16>(tag_start + 0x70));
      block.lifeTime = TRY(reader.tryGetAt<u16>(tag_start + 0x72));
      block.volumeSize = TRY(reader.tryGetAt<u16>(tag_start + 0x74));
      block.divNumber = TRY(reader.tryGetAt<u16>(tag_start + 0x76));
      block.rateStep = TRY(reader.tryGetAt<u8>(tag_start + 0x78));

      block.moment = 1;

      resource.bem1 = block;
    } else if (tag_name == 'BSP1') {
      // JPABaseShape
      // Contains particle draw settings.

      JPABaseShapeBlock block = JPABaseShapeBlock();

      u32 flags = TRY(reader.tryGetAt<u32>(tag_start + 0x08));
      block.shapeType = static_cast<ShapeType>((flags >> 0) & 0x0F);
      block.dirType = static_cast<DirType>((flags >> 4) & 0x07);
      block.rotType = static_cast<RotType>((flags >> 7) & 0x07);
      block.planeType = static_cast<PlaneType>((flags >> 10) & 0x01);
      // 11 = unk
      block.isGlblClrAnm = !!((flags >> 12) & 0x01);
      // 13 = unk
      block.isGlblTexAnm = !!((flags >> 14) & 0x01);
      block.colorInSelect = (flags >> 15) & 0x07;
      block.alphaInSelect = (flags >> 18) & 0x01;
      // 19 = unk
      block.isEnableProjection = !!((flags >> 20) & 0x01);
      block.isDrawFwdAhead = !!((flags >> 21) & 0x01);
      block.isDrawPrntAhead = !!((flags >> 22) & 0x01);
      // 23 = unk
      block.isEnableTexScrollAnm = !!((flags >> 24) & 0x01);
      block.tilingS = !!((flags >> 25) & 0x01) ? 2.0 : 1.0;
      block.tilingT = !!((flags >> 26) & 0x01) ? 2.0 : 1.0;
      block.isNoDrawParent = !!((flags >> 27) & 0x01);
      block.isNoDrawChild = !!((flags >> 28) & 0x01);

      if (block.shapeType == ShapeType::DirectionCross ||
          block.shapeType == ShapeType::RotationCross)
        block.planeType = PlaneType::X;

      f32 baseSizeX = TRY(reader.tryGetAt<f32>(tag_start + 0x10));
      f32 baseSizeY = TRY(reader.tryGetAt<f32>(tag_start + 0x14));
      block.baseSize = glm::vec2(baseSizeX, baseSizeY);

      u16 blendModeFlags = TRY(reader.tryGetAt<u16>(tag_start + 0x18));

      block.blendMode = static_cast<gx::BlendModeType>(blendModeFlags & 0x3);
      block.blendSrcFactor = static_cast<gx::BlendModeFactor>(
        (blendModeFlags >> 2) & 0x7);
      block.blendDstFactor = static_cast<gx::BlendModeFactor>(
        (blendModeFlags >> 6) & 0x7);

      u8 alphaCompareFlags = TRY(reader.tryGetAt<u8>(tag_start + 0x1A));

      block.alphaCmp0 = static_cast<gx::Comparison>(alphaCompareFlags & 0x7);
      block.alphaOp = static_cast<gx::AlphaOp>((alphaCompareFlags >> 3) & 0x3);
      block.alphaCmp1 =
          static_cast<gx::Comparison>((alphaCompareFlags >> 5) & 0x7);

      block.alphaRef0 = TRY(reader.tryGetAt<u8>(tag_start + 0x1B));
      block.alphaRef1 = TRY(reader.tryGetAt<u8>(tag_start + 0x1C));

      u8 zModeFlags = TRY(reader.tryGetAt<u8>(tag_start + 0x1D));

      block.zTest = static_cast<bool>(zModeFlags & 0x1);
      block.zCompare = static_cast<gx::Comparison>((zModeFlags >> 1) & 0x7);
      block.zWrite = static_cast<bool>((zModeFlags >> 4) & 0x1);

      u8 texFlags = TRY(reader.tryGetAt<u8>(tag_start + 0x1E));
      u8 texIdxAnimDataCount = TRY(reader.tryGetAt<u8>(tag_start + 0x1F));
      block.texIdx = TRY(reader.tryGetAt<u8>(tag_start + 0x20));
      u8 colorFlags = TRY(reader.tryGetAt<u8>(tag_start + 0x21));

      block.colorPrm = gx::Color(TRY(
          reader.tryGetAt<u32, oishii::EndianSelect::Current,
          true>(tag_start + 0x26)));
      block.colorEnv = gx::Color(
          TRY(reader.tryGetAt<u32, oishii::EndianSelect::Current, true>(
                tag_start + 0x2A)));

      block.texCalcIdxType = static_cast<CalcIdxType>((texFlags >> 2) & 0x07);

      block.anmRndm = TRY(reader.tryGetAt<u8>(tag_start + 0x2E));
      block.colorLoopOfstMask = TRY(reader.tryGetAt<u8>(tag_start + 0x2F));
      block.texIdxLoopOfstMask = TRY(reader.tryGetAt<u8>(tag_start + 0x30));

      u32 extraDataOffs = tag_start + 0x34;

      if (block.isEnableTexScrollAnm) {
        block.texInitTransX = TRY(reader.tryGetAt<f32>(extraDataOffs + 0x00));
        block.texInitTransY = TRY(reader.tryGetAt<f32>(extraDataOffs + 0x04));
        block.texInitScaleX = TRY(reader.tryGetAt<f32>(extraDataOffs + 0x08));
        block.texInitScaleY = TRY(reader.tryGetAt<f32>(extraDataOffs + 0x0C));
        block.texInitRot = TRY(reader.tryGetAt<f32>(extraDataOffs + 0x10));
        block.texIncTransX = TRY(reader.tryGetAt<f32>(extraDataOffs + 0x14));
        block.texIncTransY = TRY(reader.tryGetAt<f32>(extraDataOffs + 0x18));
        block.texIncScaleX = TRY(reader.tryGetAt<f32>(extraDataOffs + 0x1C));
        block.texIncScaleY = TRY(reader.tryGetAt<f32>(extraDataOffs + 0x20));
        block.texIncRot = TRY(reader.tryGetAt<f32>(extraDataOffs + 0x24));
        extraDataOffs += 0x28;
      }

      bool isEnableTextureAnm = !!((texFlags >> 0) & 0x01);
      if (isEnableTextureAnm) {
        reader.seek(extraDataOffs);
        for (u32 x = 0; x < texIdxAnimDataCount; x++) {
          block.texIdxAnimData.push_back(TRY(reader.tryRead<u8>()));
        }

      }

      block.colorAnimMaxFrm = TRY(reader.tryGetAt<u16>(tag_start + 0x24));

      bool isColorPrmAnm = !!((colorFlags >> 1) & 0x01);
      bool isColorEnvAnm = !!((colorFlags >> 3) & 0x01);
      block.colorCalcIdxType = static_cast<CalcIdxType>(
        (colorFlags >> 4) & 0x07);

      if (isColorPrmAnm) {
        u32 colorPrmAnimDataOffs = tag_start + TRY(
                                       reader.tryGetAt<u16>(tag_start + 0x0C));
        u32 colorPrmAnimDataCount = TRY(reader.tryGetAt<u8>(tag_start + 0x22));

        TRY(makeColorTable(reader, colorPrmAnimDataOffs, colorPrmAnimDataCount,
          block.colorPrmAnimData));
      }

      if (isColorEnvAnm) {

        u32 colorEnvAnimDataOffs =
            tag_start + TRY(reader.tryGetAt<u16>(tag_start + 0x0E));
        u32 colorEnvAnimDataCount = TRY(reader.tryGetAt<u8>(tag_start + 0x23));

        TRY(makeColorTable(reader, colorEnvAnimDataOffs, colorEnvAnimDataCount,
          block.colorEnvAnimData));
      }

      block.isEnableTexture = true;

      resource.bsp1 = block;

    } else if (tag_name == 'ESP1') {

      JPAExtraShapeBlock block = JPAExtraShapeBlock();

      u32 flags = TRY(reader.tryGetAt<u32>(tag_start + 0x08));
      block.isEnableScale = !!((flags >> 0) & 0x01);
      block.isDiffXY = !!((flags >> 1) & 0x01);
      block.scaleAnmTypeX = static_cast<CalcScaleAnmType>((flags >> 8) & 0x03);
      block.scaleAnmTypeY = static_cast<CalcScaleAnmType>((flags >> 10) & 0x03);
      block.pivotX = (flags >> 12) & 0x03;
      block.pivotY = (flags >> 14) & 0x03;
      block.isEnableAlpha = !!((flags >> 16) & 0x01);
      block.isEnableSinWave = !!((flags >> 17) & 0x01);
      block.isEnableRotate = !!((flags >> 24) & 0x01);
      block.alphaWaveType = CalcAlphaWaveType::NrmSin;
      // isEnableScaleBySpeedX was removed in JPA 2.0.
      block.isEnableScaleBySpeedX = false;
      // isEnableScaleBySpeedY was removed in JPA 2.0.
      block.isEnableScaleBySpeedY = false;

      block.scaleInTiming = TRY(reader.tryGetAt<f32>(tag_start + 0x0C));
      block.scaleOutTiming = TRY(reader.tryGetAt<f32>(tag_start + 0x10));
      block.scaleInValueX = TRY(reader.tryGetAt<f32>(tag_start + 0x14));
      block.scaleOutValueX = TRY(reader.tryGetAt<f32>(tag_start + 0x18));
      block.scaleInValueY = TRY(reader.tryGetAt<f32>(tag_start + 0x1C));
      block.scaleOutValueY = TRY(reader.tryGetAt<f32>(tag_start + 0x20));
      block.scaleOutRandom = TRY(reader.tryGetAt<f32>(tag_start + 0x24));
      block.scaleAnmMaxFrameX = TRY(reader.tryGetAt<u16>(tag_start + 0x28));
      block.scaleAnmMaxFrameY = TRY(reader.tryGetAt<u16>(tag_start + 0x2A));

      block.scaleIncreaseRateX = 1, block.scaleIncreaseRateY = 1;
      if (block.scaleInTiming > 0) {
        block.scaleIncreaseRateX =
            (1.0 - block.scaleInValueX) / block.scaleInTiming;
        block.scaleIncreaseRateY =
            (1.0 - block.scaleInValueY) / block.scaleInTiming;
      }

      block.scaleDecreaseRateX = 1, block.scaleDecreaseRateY = 1;
      if (block.scaleOutTiming < 1) {
        block.scaleDecreaseRateX =
            (block.scaleOutValueX - 1.0) / (1.0 - block.scaleOutTiming);
        block.scaleDecreaseRateY =
            (block.scaleOutValueY - 1.0) / (1.0 - block.scaleOutTiming);
      }

      block.alphaInTiming = TRY(reader.tryGetAt<f32>(tag_start + 0x2C));
      block.alphaOutTiming = TRY(reader.tryGetAt<f32>(tag_start + 0x30));
      block.alphaInValue = TRY(reader.tryGetAt<f32>(tag_start + 0x34));
      block.alphaBaseValue = TRY(reader.tryGetAt<f32>(tag_start + 0x38));
      block.alphaOutValue = TRY(reader.tryGetAt<f32>(tag_start + 0x3C));

      block.alphaDecreaseRate = 1;
      if (block.alphaInTiming > 0)
        block.alphaIncreaseRate =
            (block.alphaBaseValue - block.alphaInValue) / block.alphaInTiming;

      block.alphaDecreaseRate = 1;
      if (block.alphaOutTiming < 1)
        block.alphaDecreaseRate =
            (block.alphaOutValue - block.alphaBaseValue) / (
              1.0f - block.alphaOutTiming);

      f32 alphaWaveFrequency = TRY(reader.tryGetAt<f32>(tag_start + 0x40));
      block.alphaWaveRandom = TRY(reader.tryGetAt<f32>(tag_start + 0x44));
      f32 alphaWaveAmplitude = TRY(reader.tryGetAt<f32>(tag_start + 0x48));

      // Put in terms of JPA1 alpha wave parameters.
      block.alphaWaveParam1 = alphaWaveFrequency;
      block.alphaWaveParam2 = 0.0;
      block.alphaWaveParam3 = alphaWaveAmplitude;

      block.rotateAngle = TRY(reader.tryGetAt<f32>(tag_start + 0x4C)) *
                          std::numbers::pi * 2 / 0xFFFF;
      block.rotateAngleRandom = TRY(reader.tryGetAt<f32>(tag_start + 0x50)) *
                                std::numbers::pi * 2 / 0xFFFF;
      block.rotateSpeed = TRY(reader.tryGetAt<f32>(tag_start + 0x54)) *
                          std::numbers::pi * 2 / 0xFFFF;
      block.rotateSpeedRandom = TRY(reader.tryGetAt<f32>(tag_start + 0x58));
      block.rotateDirection = TRY(reader.tryGetAt<f32>(tag_start + 0x5C));

      resource.esp1.emplace(block);
    } else if (tag_name == 'SSP1') {
      // JPAChildShape / JPASweepShape
      // Contains child particle draw settings.
      JPAChildShapeBlock block = JPAChildShapeBlock();

      u32 flags = TRY(reader.tryGetAt<u32>(tag_start + 0x08));
      block.shapeType = static_cast<ShapeType>((flags >> 0) & 0x0F);
      block.dirType = static_cast<DirType>((flags >> 4) & 0x07);
      block.rotType = static_cast<RotType>((flags >> 7) & 0x07);
      block.planeType = static_cast<PlaneType>((flags >> 10) & 0x01);
      block.isInheritedScale = ((flags >> 16) & 0x01);
      block.isInheritedAlpha = ((flags >> 17) & 0x01);
      block.isInheritedRGB = ((flags >> 18) & 0x01);
      block.isEnableField = ((flags >> 21) & 0x01);
      block.isEnableScaleOut = ((flags >> 22) & 0x01);
      block.isEnableAlphaOut = ((flags >> 23) & 0x01);
      block.isEnableRotate = ((flags >> 24) & 0x01);

      if (block.shapeType == ShapeType::DirectionCross ||
          block.shapeType == ShapeType::RotationCross)
        block.planeType = PlaneType::X;

      block.posRndm = TRY(reader.tryGetAt<f32>(tag_start + 0x0C));
      block.baseVel = TRY(reader.tryGetAt<f32>(tag_start + 0x10));
      block.baseVelRndm = TRY(reader.tryGetAt<f32>(tag_start + 0x14));
      block.velInfRate = TRY(reader.tryGetAt<f32>(tag_start + 0x18));
      block.gravity = TRY(reader.tryGetAt<f32>(tag_start + 0x1C));

      f32 globalScale2DX = TRY(reader.tryGetAt<f32>(tag_start + 0x20));
      f32 globalScale2DY = TRY(reader.tryGetAt<f32>(tag_start + 0x24));
      block.globalScale2D = glm::vec2(globalScale2DX, globalScale2DY);

      block.inheritScale = TRY(reader.tryGetAt<f32>(tag_start + 0x28));
      block.inheritAlpha = TRY(reader.tryGetAt<f32>(tag_start + 0x2C));
      block.inheritRGB = TRY(reader.tryGetAt<f32>(tag_start + 0x30));
      block.colorPrm =
          colorFromRGBA8(TRY(reader.tryGetAt<u32>(tag_start + 0x34)));
      block.colorEnv =
          colorFromRGBA8(TRY(reader.tryGetAt<u32>(tag_start + 0x38)));
      block.timing = TRY(reader.tryGetAt<f32>(tag_start + 0x3C));
      block.life = TRY(reader.tryGetAt<u16>(tag_start + 0x40));
      block.rate = TRY(reader.tryGetAt<u16>(tag_start + 0x42));
      block.step = TRY(reader.tryGetAt<u8>(tag_start + 0x44));
      block.texIdx = TRY(reader.tryGetAt<u8>(tag_start + 0x45));
      block.rotateSpeed = TRY(reader.tryGetAt<u16>(tag_start + 0x46)) / 0xFFFF;

      resource.ssp1.emplace(block);

    } else if (tag_name == 'FLD1') {

      JPAFieldBlock block = JPAFieldBlock();

      u32 flags = TRY(reader.tryGetAt<u32>(tag_start + 0x08));
      block.type = static_cast<FieldType>((flags >> 0) & 0x0F);
      block.addType = static_cast<FieldAddType>((flags >> 8) & 0x03);
      block.sttFlag = (flags >> 16);

      f32 posX = TRY(reader.tryGetAt<f32>(tag_start + 0x0C));
      f32 posY = TRY(reader.tryGetAt<f32>(tag_start + 0x10));
      f32 posZ = TRY(reader.tryGetAt<f32>(tag_start + 0x14));
      block.pos = glm::vec3(posX, posY, posZ);

      f32 dirX = TRY(reader.tryGetAt<f32>(tag_start + 0x18));
      f32 dirY = TRY(reader.tryGetAt<f32>(tag_start + 0x1C));
      f32 dirZ = TRY(reader.tryGetAt<f32>(tag_start + 0x20));
      block.dir = glm::vec3(dirX, dirY, dirZ);

      f32 param1 = TRY(reader.tryGetAt<f32>(tag_start + 0x24));
      f32 param2 = TRY(reader.tryGetAt<f32>(tag_start + 0x28));
      f32 param3 = TRY(reader.tryGetAt<f32>(tag_start + 0x2C));

      block.fadeIn = TRY(reader.tryGetAt<f32>(tag_start + 0x30));
      block.fadeOut = TRY(reader.tryGetAt<f32>(tag_start + 0x34));
      block.enTime = TRY(reader.tryGetAt<f32>(tag_start + 0x38));
      block.disTime = TRY(reader.tryGetAt<f32>(tag_start + 0x3C));
      block.cycle = TRY(reader.tryGetAt<u8>(tag_start + 0x40));

      block.fadeInRate = 1;
      if (block.fadeIn > block.enTime)
        block.fadeInRate = 1 / (block.fadeIn - block.enTime);

      block.fadeOutRate = 1;
      if (block.fadeOut < block.disTime)
        block.fadeOutRate = 1 / (block.disTime - block.fadeOut);

      block.refDistance = -1;
      block.innerSpeed = -1;
      block.outerSpeed = -1;

      block.mag = param1;
      block.magRndm = 0;

      if (block.type == FieldType::Newton) {
        block.refDistance = param3 * param3;
      }

      if (block.type == FieldType::Vortex) {
        block.innerSpeed = param1;
        block.outerSpeed = param2;
      }

      if (block.type == FieldType::Air) {
        block.refDistance = param2;
      }

      if (block.type == FieldType::Convection) {
        block.refDistance = param3;
      }

      if (block.type == FieldType::Spin) {
        block.innerSpeed = param1;
      }
      resource.fld1.push_back(block);
    } else if (tag_name == 'TDB1') {
      resource.tdb1 = TRY(reader.tryReadBuffer<u16>(tdb1Count,tag_start + 0x8));
    }

    reader.seek(tag_start + tag_size);
  }
  resources.push_back(resource);
  return {};
}


Result<void> JPAC::load_block_data_from_file(oishii::BinaryReader& reader,
                                             s32 tag_count) {


  JPAResource resource = JPAResource();

  u32 texID = 0;

  for (s32 i = 0; i < tag_count; ++i) {
    auto tag_start = reader.tell();

    auto tag_name = TRY(reader.tryRead<s32>());
    auto tag_size = TRY(reader.tryRead<s32>());

    u32 kfa1KeyTypeMask = 0;

    if (tag_name == 'BEM1') {
      JPADynamicsBlock block = JPADynamicsBlock();
      f32 emitterSclX = TRY(reader.tryGetAt<f32>(tag_start + 0xC));
      f32 emitterSclY = TRY(reader.tryGetAt<f32>(tag_start + 0x10));
      f32 emitterSclZ = TRY(reader.tryGetAt<f32>(tag_start + 0x14));
      block.emitterScl = glm::vec3(emitterSclX, emitterSclY, emitterSclZ);

      f32 emitterTrsX = TRY(reader.tryGetAt<f32>(tag_start + 0x18));
      f32 emitterTrsY = TRY(reader.tryGetAt<f32>(tag_start + 0x1C));
      f32 emitterTrsZ = TRY(reader.tryGetAt<f32>(tag_start + 0x20));
      block.emitterTrs = glm::vec3(emitterTrsX, emitterTrsY, emitterTrsZ);

      // Tau is 2 * PI
      f32 emitterRotX =
          (TRY(reader.tryGetAt<u16>(tag_start + 0x24)) / 0x7FFF) * (
            std::numbers::pi);
      f32 emitterRotY = (TRY(reader.tryGetAt<u16>(tag_start + 0x26)) / 0x7FFF) *
                        (std::numbers::pi);
      f32 emitterRotZ = (TRY(reader.tryGetAt<u16>(tag_start + 0x28)) / 0x7FFF) *
                        (std::numbers::pi);
      block.emitterRot = glm::vec3(emitterRotX, emitterRotY, emitterRotZ);

      block.volumeType = {TRY(reader.tryGetAt<u8>(tag_start + 0x2A))};
      block.rateStep = TRY(reader.tryGetAt<u8>(tag_start + 0x2B));
      block.divNumber = TRY(reader.tryGetAt<u16>(tag_start + 0x2E));

      block.rate = TRY(reader.tryGetAt<f32>(tag_start + 0x30));
      block.rateRndm =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x34)));
      block.maxFrame = TRY(reader.tryGetAt<u16>(tag_start + 0x36));
      block.startFrame = TRY(reader.tryGetAt<u16>(tag_start + 0x38));
      block.volumeSize = TRY(reader.tryGetAt<u16>(tag_start + 0x3A));
      block.volumeSweep = JPAConvertFixToFloat(
          TRY(reader.tryGetAt<u16>(tag_start + 0x3C)));
      block.volumeMinRad =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x3E)));
      block.lifeTime = TRY(reader.tryGetAt<u16>(tag_start + 0x40));
      block.lifeTimeRndm =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x42)));
      block.moment =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x44)));
      block.momentRndm =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x46)));
      block.initialVelRatio =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x48)));
      block.accelRndm =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x4A)));
      block.airResist =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x4C)));
      block.airResistRndm =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x4E)));
      block.initialVelOmni = TRY(reader.tryGetAt<f32>(tag_start + 0x50));
      block.initialVelAxis = TRY(reader.tryGetAt<f32>(tag_start + 0x54));
      block.initialVelRndm = TRY(reader.tryGetAt<f32>(tag_start + 0x58));
      block.initialVelDir = TRY(reader.tryGetAt<f32>(tag_start + 0x5c));
      block.accel = TRY(reader.tryGetAt<f32>(tag_start + 0x60));

      f32 emitterDirX = JPAConvertFixToFloat(
          TRY(reader.tryGetAt<u16>(tag_start + 0x64)));
      f32 emitterDirY = JPAConvertFixToFloat(
          TRY(reader.tryGetAt<u16>(tag_start + 0x66)));
      f32 emitterDirZ = JPAConvertFixToFloat(
          TRY(reader.tryGetAt<u16>(tag_start + 0x68)));
      block.emitterDir = glm::normalize(
          glm::vec3(emitterDirX, emitterDirY, emitterDirZ));

      block.spread = JPAConvertFixToFloat(
          TRY(reader.tryGetAt<u16>(tag_start + 0x6A)));
      block.emitFlags = TRY(reader.tryGetAt<u32>(tag_start + 0x6C));

      kfa1KeyTypeMask = TRY(reader.tryGetAt<u32>(tag_start + 0x70));;
      resource.bem1 = block;
    } else if (tag_name == 'BSP1') {
      JPABaseShapeBlock block = JPABaseShapeBlock();

      // JPABaseShape
      // Contains particle draw settings.

      f32 baseSizeX = TRY(reader.tryGetAt<f32>(tag_start + 0x18));
      f32 baseSizeY = TRY(reader.tryGetAt<f32>(tag_start + 0x1C));
      block.baseSize = glm::vec2(baseSizeX, baseSizeY);

      block.anmRndm = TRY(reader.tryGetAt<u16>(tag_start + 0x20));
      u8 texAnmCalcFlags = TRY(reader.tryGetAt<u8>(tag_start + 0x22));
      u8 colorAnmCalcFlags = TRY(reader.tryGetAt<u8>(tag_start + 0x23));

      block.texIdxLoopOfstMask = ((texAnmCalcFlags >> 0) & 0x01)
                                   ? 0xFFFF
                                   : 0x0000;
      block.isGlblTexAnm = !!((texAnmCalcFlags >> 1) & 0x01);


      block.colorLoopOfstMask =
          !!((colorAnmCalcFlags >> 0) & 0x01) ? 0xFFFF : 0x0000;
      block.isGlblClrAnm = !!((colorAnmCalcFlags >> 1) & 0x01);

      block.shapeType = {TRY(reader.tryGetAt<u8>(tag_start + 0x24))};
      block.dirType = {TRY(reader.tryGetAt<u8>(tag_start + 0x25))};
      block.rotType = {TRY(reader.tryGetAt<u8>(tag_start + 0x26))};

      // planeType does not exist in JEFFjpa1.
      block.planeType = PlaneType::XY;

      // stopDrawParent is in the SSP1 block in JEFFjpa1.
      block.isNoDrawParent = false;
      // stopDrawChild does not exist in JEFFjpa1.
      block.isNoDrawChild = false;

      block.colorInSelect = TRY(reader.tryGetAt<u8>(tag_start + 0x30));

      // alphaInSelect was added in JEFFjpa1.
      block.alphaInSelect = 0;

      // Will pack into bitfields for JPAC
      block.blendMode = {TRY(reader.tryGetAt<u8>(tag_start + 0x35))};
      block.blendSrcFactor = {TRY(reader.tryGetAt<u8>(tag_start + 0x36))};
      block.blendDstFactor = {TRY(reader.tryGetAt<u8>(tag_start + 0x37))};
      block.logicOp = {TRY(reader.tryGetAt<u8>(tag_start + 0x38))};

      block.alphaCmp0 = {TRY(reader.tryGetAt<u8>(tag_start + 0x39))};
      block.alphaRef0 = TRY(reader.tryGetAt<u8>(tag_start + 0x3A));
      block.alphaOp = {TRY(reader.tryGetAt<u8>(tag_start + 0x3B))};
      block.alphaCmp1 = {TRY(reader.tryGetAt<u8>(tag_start + 0x3C))};
      block.alphaRef1 = TRY(reader.tryGetAt<u8>(tag_start + 0x3D));

      // 0x3E is ZCompLoc
      block.zTest = TRY(reader.tryGetAt<u8>(tag_start + 0x3F));
      block.zCompare = {TRY(reader.tryGetAt<u8>(tag_start + 0x40))};
      block.zWrite = TRY(reader.tryGetAt<u8>(tag_start + 0x41));

      block.isEnableProjection = TRY(reader.tryGetAt<u8>(tag_start + 0x43));

      u8 flags = TRY(reader.tryGetAt<u8>(tag_start + 0x44));

      u8 texAnimFlags = TRY(reader.tryGetAt<u8>(tag_start + 0x4C));
      block.texCalcIdxType = {TRY(reader.tryGetAt<u8>(tag_start + 0x4D))};
      block.texIdx = TRY(reader.tryGetAt<u8>(tag_start + 0x4F));

      if (((texAnimFlags >> 0) & 0x01)) {
        u16 texIdxAnimDataOffs = tag_start + TRY(
                                     reader.tryGetAt<u16>(tag_start + 0x12));
        uint32_t texIdxAnimDataCount = TRY(
            reader.tryGetAt<u8>(tag_start + 0x4E));

        reader.seek(texIdxAnimDataOffs);
        for (u32 x = 0; x < texIdxAnimDataCount; x++) {
          block.texIdxAnimData.push_back(TRY(reader.tryRead<u8>()));
        }

      }

      block.colorAnimMaxFrm = TRY(reader.tryGetAt<u16>(tag_start + 0x5C));
      block.colorCalcIdxType = {TRY(reader.tryGetAt<u8>(tag_start + 0x5E))};
      u8 colorPrmAnimFlags = TRY(reader.tryGetAt<u8>(tag_start + 0x60));
      u8 colorEnvAnimFlags = TRY(reader.tryGetAt<u8>(tag_start + 0x61));

      if (((colorPrmAnimFlags >> 2) & 0x01)) {
        u16 colorPrmAnimDataOffs = tag_start + TRY(
                                       reader.tryGetAt<u16>(tag_start + 0x14));
        u8 colorPrmAnimDataCount = TRY(reader.tryGetAt<u8>(tag_start + 0x62));

        TRY(makeColorTable(reader, colorPrmAnimDataOffs, colorPrmAnimDataCount,
          block.colorPrmAnimData));
      }

      if (((colorEnvAnimFlags >> 2) & 0x01)) {
        u16 colorEnvAnimDataOffs = tag_start + TRY(
                                       reader.tryGetAt<u16>(tag_start + 0x16));
        u8 colorEnvAnimDataCount = TRY(reader.tryGetAt<u8>(tag_start + 0x63));

        TRY(makeColorTable(reader, colorEnvAnimDataOffs, colorEnvAnimDataCount,
          block.colorEnvAnimData));
      }

      block.colorPrm = colorFromRGBA8(
          TRY(reader.tryGetAt<u32>(tag_start + 0x64)));
      block.colorEnv =
          colorFromRGBA8(TRY(reader.tryGetAt<u32>(tag_start + 0x68)));

      block.texInitTransX =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x80))) *
          10;
      block.texInitTransY =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x82))) *
          10;
      block.texInitScaleX =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x84))) *
          10;
      block.texInitScaleY =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x86))) *
          10;
      block.tilingS =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x88))) *
          10;
      block.tilingT =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x8A))) *
          10;
      block.texIncTransX =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x8C)));
      block.texIncTransY =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x8E)));
      block.texIncScaleX =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x90))) *
          0.1f;
      block.texIncScaleY =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x92))) *
          0.1f;
      block.texIncRot =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x94)));
      // texStaticRotate was added in JPA2.
      block.texInitRot = 0;

      block.isEnableTexScrollAnm = TRY(reader.tryGetAt<u16>(tag_start + 0x96));

      block.isDrawFwdAhead = ((flags >> 0) & 0x01);
      block.isDrawPrntAhead = ((flags >> 1) & 0x01);

      block.isEnableTexture = true;

      resource.bsp1 = block;
    } else if (tag_name == 'ESP1') {
      // JPAExtraShape
      // Contains misc. extra particle draw settings.

      JPAExtraShapeBlock block = JPAExtraShapeBlock();

      block.alphaInTiming =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x14)));
      block.alphaOutTiming =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x16)));
      block.alphaInValue = JPAConvertFixToFloat(
          TRY(reader.tryGetAt<u16>(tag_start + 0x18)));
      block.alphaBaseValue =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x1A)));
      block.alphaOutValue =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x1C)));
      u8 alphaAnmFlags = TRY(reader.tryGetAt<u8>(tag_start + 0x1E));

      block.isEnableAlpha = ((alphaAnmFlags >> 0) & 0x01);
      block.isEnableSinWave = ((alphaAnmFlags >> 1) & 0x01);
      u8 alphaWaveTypeFlag = TRY(reader.tryGetAt<u8>(tag_start + 0x1F));
      if (block.isEnableSinWave) {
        block.alphaWaveType = static_cast<CalcAlphaWaveType>(
          alphaWaveTypeFlag);
      } else {
        // default value of 0
        block.alphaWaveType = CalcAlphaWaveType::NrmSin;
      }

      block.alphaIncreaseRate = 1;
      if (block.alphaInTiming > 0)
        block.alphaIncreaseRate =
            (block.alphaBaseValue - block.alphaInValue) / block.
            alphaInTiming;

      block.alphaDecreaseRate = 1;
      if (block.alphaOutTiming < 1)
        block.alphaDecreaseRate =
            (block.alphaOutValue - block.alphaBaseValue) /
            (1.0 - block.alphaOutTiming);

      block.alphaWaveParam1 =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x20)));
      block.alphaWaveParam2 =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x22)));
      block.alphaWaveParam3 =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x24)));
      block.alphaWaveRandom =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x26)));

      block.scaleOutRandom =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x34)));
      block.scaleInTiming =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x36)));
      block.scaleOutTiming =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x38)));

      block.scaleInValueY =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x3A)));
      block.scaleOutValueY =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x3E)));
      block.pivotY = TRY(reader.tryGetAt<u8>(tag_start + 0x40));
      u8 anmTypeY = TRY(reader.tryGetAt<u8>(tag_start + 0x41));
      block.scaleAnmMaxFrameY = TRY(reader.tryGetAt<u16>(tag_start + 0x42));

      block.scaleInValueX =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x44)));
      block.scaleOutValueX =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x48)));
      block.pivotX = TRY(reader.tryGetAt<u8>(tag_start + 0x4A));
      u8 anmTypeX = TRY(reader.tryGetAt<u8>(tag_start + 0x4B));
      block.scaleAnmMaxFrameX = TRY(reader.tryGetAt<u16>(tag_start + 0x4C));

      u8 scaleAnmFlags = TRY(reader.tryGetAt<u8>(tag_start + 0x4E));

      block.isEnableScale = ((scaleAnmFlags >> 1) & 0x01);
      block.isDiffXY = ((scaleAnmFlags >> 2) & 0x01);
      bool isEnableScaleAnmY = ((scaleAnmFlags >> 3) & 0x01);
      bool isEnableScaleAnmX = ((scaleAnmFlags >> 4) & 0x01);
      block.isEnableScaleBySpeedY = ((scaleAnmFlags >> 5) & 0x01);
      block.isEnableScaleBySpeedX = ((scaleAnmFlags >> 6) & 0x01);
      block.scaleAnmTypeX =
          isEnableScaleAnmX
            ? anmTypeX
                ? CalcScaleAnmType::Reverse
                : CalcScaleAnmType::Repeat
            : CalcScaleAnmType::Normal;
      block.scaleAnmTypeY =
          isEnableScaleAnmY
            ? anmTypeY
                ? CalcScaleAnmType::Reverse
                : CalcScaleAnmType::Repeat
            : CalcScaleAnmType::Normal;

      block.scaleIncreaseRateX = 1, block.scaleIncreaseRateY = 1;
      if (block.scaleInTiming > 0) {
        block.scaleIncreaseRateX =
            (1.0 - block.scaleInValueX) / block.scaleInTiming;
        block.scaleIncreaseRateY =
            (1.0 - block.scaleInValueY) / block.scaleInTiming;
      }

      block.scaleDecreaseRateX = 1, block.scaleDecreaseRateY = 1;
      if (block.scaleOutTiming < 1) {
        block.scaleDecreaseRateX =
            (block.scaleOutValueX - 1.0) / (1.0 - block.scaleOutTiming);
        block.scaleDecreaseRateY =
            (block.scaleOutValueY - 1.0) / (1.0 - block.scaleOutTiming);
      }

      block.rotateAngle = JPAConvertFixToFloat(
                              TRY(reader.tryGetAt<u16>(tag_start + 0x5A))) *
                          std::numbers::pi * 2.0f;
      block.rotateSpeed = JPAConvertFixToFloat(
                              TRY(reader.tryGetAt<u16>(tag_start + 0x5C))) *
                          std::numbers::pi * 2.0f;
      block.rotateAngleRandom =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x5E))) *
          std::numbers::pi * 2.0f;
      block.rotateSpeedRandom =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x60)));
      block.rotateDirection =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x62)));
      block.isEnableRotate = TRY(reader.tryGetAt<u8>(tag_start + 0x64));

      resource.esp1.emplace(block);
    } else if (tag_name == 'SSP1') {
      // JPAChildShape / JPASweepShape
      // Contains child particle draw settings.
      JPAChildShapeBlock block = JPAChildShapeBlock();

      block.shapeType = {TRY(reader.tryGetAt<u8>(tag_start + 0x10))};
      block.dirType = {TRY(reader.tryGetAt<u8>(tag_start + 0x11))};
      block.rotType = {TRY(reader.tryGetAt<u8>(tag_start + 0x12))};

      // planeType does not exist in JEFFjpa1.
      block.planeType = PlaneType::XY;

      block.life = TRY(reader.tryGetAt<u16>(tag_start + 0x14));
      block.rate = TRY(reader.tryGetAt<u16>(tag_start + 0x16));
      block.step = TRY(reader.tryGetAt<u8>(tag_start + 0x1A));
      block.posRndm = TRY(reader.tryGetAt<f32>(tag_start + 0x28));
      block.baseVel = TRY(reader.tryGetAt<f32>(tag_start + 0x2C));
      block.isEnableField = TRY(reader.tryGetAt<u8>(tag_start + 0x36));

      bool isEnableDrawParent = TRY(reader.tryGetAt<u8>(tag_start + 0x44));
      resource.bsp1.isNoDrawParent = !isEnableDrawParent;
      // EXPECT(resource.bsp1.isNoDrawParent == !isEnableDrawParent);

      block.isEnableScaleOut = TRY(reader.tryGetAt<u8>(tag_start + 0x45));
      block.isEnableAlphaOut = TRY(reader.tryGetAt<u8>(tag_start + 0x46));
      block.texIdx = TRY(reader.tryGetAt<u8>(tag_start + 0x47));

      f32 globalScale2DX = TRY(reader.tryGetAt<f32>(tag_start + 0x4C));
      f32 globalScale2DY = TRY(reader.tryGetAt<f32>(tag_start + 0x50));
      block.globalScale2D = glm::vec2(globalScale2DX, globalScale2DY);

      block.isEnableRotate = TRY(reader.tryGetAt<u8>(tag_start + 0x56));
      u8 flags = TRY(reader.tryGetAt<u8>(tag_start + 0x57));
      block.isInheritedScale = ((flags >> 0) & 0x01);
      block.isInheritedAlpha = ((flags >> 1) & 0x01);
      block.isInheritedRGB = ((flags >> 2) & 0x01);

      block.colorPrm = gx::Color(TRY(reader.tryGetAt<u32>(tag_start + 0x58)));
      block.colorEnv = gx::Color(TRY(reader.tryGetAt<u32>(tag_start + 0x5C)));

      block.timing = JPAConvertFixToFloat(
          TRY(reader.tryGetAt<u16>(tag_start + 0x18)));
      block.velInfRate = JPAConvertFixToFloat(
          TRY(reader.tryGetAt<u16>(tag_start + 0x30)));
      block.baseVelRndm = JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x32)));
      block.gravity = JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x34)));
      block.inheritScale =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x48)));
      block.inheritAlpha =
          JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x4a)));
      block.inheritRGB = JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x60)));
      block.rotateSpeed = JPAConvertFixToFloat(TRY(reader.tryGetAt<u16>(tag_start + 0x54)));

      resource.ssp1.emplace(block);

    } else if (tag_name == 'ETX1') {

      JPAExTexBlock block = JPAExTexBlock();

      // unknown
      u32 flags = TRY(reader.tryGetAt<u32>(tag_start + 0x00));

      f32 p00 = TRY(reader.tryGetAt<f32>(tag_start + 0x04));
      f32 p01 = TRY(reader.tryGetAt<f32>(tag_start + 0x08));
      f32 p02 = TRY(reader.tryGetAt<f32>(tag_start + 0x0C));
      f32 p10 = TRY(reader.tryGetAt<f32>(tag_start + 0x10));
      f32 p11 = TRY(reader.tryGetAt<f32>(tag_start + 0x14));
      f32 p12 = TRY(reader.tryGetAt<f32>(tag_start + 0x18));
      f32 scale = std::pow(2, TRY(reader.tryGetAt<u8>(tag_start + 0x1C)));
      block.indTextureMtx = glm::mat2x4(0.0f);
      block.indTextureMtx[0][0] = p00 * scale;
      block.indTextureMtx[0][1] = p01 * scale;
      block.indTextureMtx[0][2] = p02 * scale;
      block.indTextureMtx[0][0] = scale;
      block.indTextureMtx[1][0] = p10 * scale;
      block.indTextureMtx[1][1] = p11 * scale;
      block.indTextureMtx[1][2] = p12 * scale;
      block.indTextureMtx[1][3] = 0.0;

      block.indTextureMode = static_cast<IndTextureMode>((flags >> 0) & 0x03);
      block.indTextureID = TRY(reader.tryGetAt<u8>(tag_start + 0x20));
      block.subTextureID = TRY(reader.tryGetAt<u8>(tag_start + 0x21));
      block.secondTextureIndex =
          (((flags >> 8) & 0x01))
            ? TRY(reader.tryGetAt<u8>(tag_start + 0x22))
            : -1;

      resource.etx1.emplace(block);

    } else if (tag_name == 'KFA1') {

      // JPAKeyBlock
      // Contains curve animations for various emitter parameters.

      JPAKeyBlock block = JPAKeyBlock();

      // assert(kfa1KeyTypeMask != 0);

      // Look for the first set bit on the right-hand side.
      JPAKeyType keyType = JPAKeyType::None;
      for (u8 i = static_cast<u8>(JPAKeyType::Rate);
           i < static_cast<u8>(JPAKeyType::Scale); i++) {
        if (kfa1KeyTypeMask & (1 << i)) {
          keyType = static_cast<JPAKeyType>(i);
          break;
        }
      }

      u8 keyCount = TRY(reader.tryGetAt<u8>(tag_start + 0x10));
      block.isLoopEnable = TRY(reader.tryGetAt<u8>(tag_start + 0x52));

      // The curves are four floats per key, in typical time/value/tangent
      // in/tangent out order.

      reader.seek(tag_start + 0x20);
      for (u32 x = 0; x < keyCount * 4; x++) {
        block.keyValues.push_back(TRY(reader.tryRead<f32>()));
      }

      // Now unset it from the mask so we don't find it again.
      kfa1KeyTypeMask = kfa1KeyTypeMask & ~(1 << static_cast<u8>(keyType));
      resource.kfa1.push_back(std::move(block));
    } else if (tag_name == 'FLD1') {

      JPAFieldBlock block = JPAFieldBlock();

      block.type = static_cast<FieldType>(TRY(
          reader.tryGetAt<u8>(tag_start + 0x0C)));
      block.addType = static_cast<FieldAddType>(TRY(
          reader.tryGetAt<u8>(tag_start + 0x0E)));
      block.cycle = TRY(reader.tryGetAt<u8>(tag_start + 0x0F));
      block.sttFlag = (
        TRY(reader.tryGetAt<u8>(tag_start + 0x10)));

      block.mag = TRY(reader.tryGetAt<f32>(tag_start + 0x14));
      block.magRndm = TRY(reader.tryGetAt<f32>(tag_start + 0x18));
      f32 maxDist = TRY(reader.tryGetAt<f32>(tag_start + 0x1C));
      block.maxDist = maxDist;

      f32 posX = TRY(reader.tryGetAt<f32>(tag_start + 0x20));
      f32 posY = TRY(reader.tryGetAt<f32>(tag_start + 0x24));
      f32 posZ = TRY(reader.tryGetAt<f32>(tag_start + 0x28));
      block.pos = glm::vec3(posX, posY, posZ);

      f32 dirX = TRY(reader.tryGetAt<f32>(tag_start + 0x2C));
      f32 dirY = TRY(reader.tryGetAt<f32>(tag_start + 0x30));
      f32 dirZ = TRY(reader.tryGetAt<f32>(tag_start + 0x34));
      block.dir = glm::vec3(dirX, dirY, dirZ);

      f32 param1 = TRY(reader.tryGetAt<f32>(tag_start + 0x38));
      f32 param2 = TRY(reader.tryGetAt<f32>(tag_start + 0x3C));
      f32 param3 = TRY(reader.tryGetAt<f32>(tag_start + 0x40));

      block.fadeIn = JPAConvertFixToFloat(
          TRY(reader.tryGetAt<u16>(tag_start + 0x44)));
      block.fadeOut = JPAConvertFixToFloat(
          TRY(reader.tryGetAt<u16>(tag_start + 0x46)));
      block.enTime = JPAConvertFixToFloat(
          TRY(reader.tryGetAt<u16>(tag_start + 0x48)));
      block.disTime = JPAConvertFixToFloat(
          TRY(reader.tryGetAt<u16>(tag_start + 0x4A)));

      block.fadeInRate = 1;
      if (block.fadeIn > block.enTime)
        block.fadeInRate = 1 / (block.fadeIn - block.enTime);

      block.fadeOutRate = 1;
      if (block.fadeOut < block.disTime)
        block.fadeOutRate = 1 / (block.disTime - block.fadeOut);

      block.refDistance = -1;
      block.innerSpeed = -1;
      block.outerSpeed = -1;

      if (block.type == FieldType::Newton) {
        block.refDistance = param1 * param1;
      }

      if (block.type == FieldType::Vortex) {
        block.innerSpeed = block.mag;
        block.outerSpeed = block.magRndm;
      }

      if (block.type == FieldType::Air) {
        block.refDistance = block.magRndm;
      }

      if (block.type == FieldType::Convection) {
        block.refDistance = param2;
      }

      if (block.type == FieldType::Spin) {
        block.innerSpeed = block.mag;
      }
      resource.fld1.push_back(block);
    } else if (tag_name == 'TEX1') {
      //
      reader.skip(0x4);
      auto name = TRY(reader.tryReadBuffer<u8>(0x14));
      librii::j3d::Tex tmp;
      auto tex_start = reader.tell();
      rsl::SafeReader safeReader(reader);
      TRY(tmp.transfer(safeReader));
      auto buffer_addr = tex_start + tmp.ofsTex;
      auto buffer_size = librii::gx::computeImageSize(
          tmp.mWidth, tmp.mHeight, tmp.mFormat, tmp.mMipmapLevel);

      auto image_data = TRY(reader.tryReadBuffer<u8>(buffer_size, buffer_addr));

      TextureBlock block = TextureBlock(tmp, image_data);
      block.setName(std::string(name.begin(), name.end()));

      textures.push_back(block);
    }

    reader.seek(tag_start + tag_size);
  }
  resources.push_back(resource);
  return {};
}


Result<JPAC> JPAC::loadFromStream(oishii::BinaryReader& reader) {
  JPAC temp = JPAC();
  auto magic = TRY(reader.tryRead<u32>());
  if (magic == 'JEFF') {
    version = 0;
    reader.seekSet(0x0c);
    blockCount = TRY(reader.tryRead<u32>());

    temp.resources = std::vector<JPAResource>();

    reader.seekSet(0x20);
    TRY(load_block_data_from_file(reader, blockCount));

    return temp;
  } else if (magic == 'JPAC') {
    auto subVersion = TRY(reader.tryRead<u32>());
    if (subVersion == '2-10') {
      version = 2;

      temp.resources = std::vector<JPAResource>();

      u16 effectCount = TRY(reader.tryRead<u16>());
      u16 textureCount = TRY(reader.tryRead<u16>());
      u32 textureTableOffs = TRY(reader.tryRead<u32>());

      u32 effectTableIdx = 0x10;
      for (int i = 0; i < effectCount; i++) {
        u32 resourceBeginOffs = effectTableIdx;

        u32 resourceId =
            TRY(reader.tryGetAt<u16, oishii::EndianSelect::Current, true>(
                  effectTableIdx + 0x00));
        u32 blockCount =
            TRY(reader.tryGetAt<u16, oishii::EndianSelect::Current, true>(
                  effectTableIdx + 0x02));

        effectTableIdx += 0x08;
        for (int j = 0; j < blockCount; j++) {
          // blockSize includes the header.
          u32 blockSize = TRY(reader.tryGetAt<u32>(effectTableIdx + 0x04));
          effectTableIdx += blockSize;
        }

        reader.seekSet(resourceBeginOffs + 0x2);
        TRY(load_block_data_from_file_JPAC2_10(reader, blockCount));

      }

      u32 textureTableIdx = textureTableOffs;
      for (int i = 0; i < textureCount; i++) {
        reader.seek(textureTableIdx);
        auto tag_name = TRY(reader.tryRead<s32>());
        assert(tag_name == 'TEX1');
        u32 blockSize = TRY(reader.tryRead<u32>());
        reader.skip(0x4);
        auto name = TRY(reader.tryReadBuffer<u8>(0x14));

        librii::j3d::Tex tmp;
        auto tex_start = reader.tell();
        std::cout << tex_start << std::endl;
        rsl::SafeReader safeReader(reader);
        TRY(tmp.transfer(safeReader));
        auto buffer_addr = tex_start + tmp.ofsTex;
        auto buffer_size = librii::gx::computeImageSize(
            tmp.mWidth, tmp.mHeight, tmp.mFormat, tmp.mMipmapLevel);

        auto image_data =
            TRY(reader.tryReadBuffer<u8>(buffer_size, buffer_addr));

        TextureBlock block = TextureBlock(tmp, image_data);
        block.setName(std::string(name.begin(), name.end()));

        textures.push_back(block);
        textureTableIdx += blockSize;
      }

      return temp;
    }
  } else {
    return std::unexpected("Not a valid JPA file");
  }
}

Result<JPAC> JPAC::loadFromFile(std::string_view fileName) {
  auto reader =
      TRY(oishii::BinaryReader::FromFilePath(fileName, std::endian::big));
  return loadFromStream(reader);
}

Result<JPAC> JPAC::loadFromMemory(std::span<const u8> buf,
                                  std::string_view filename) {
  oishii::BinaryReader reader(buf, filename, std::endian::big);
  return loadFromStream(reader);
}

Result<JPAC> JPAC::fromFile(std::string_view path) {
  JPAC bin;
  TRY(bin.loadFromFile(path));
  return bin;
}

Result<JPAC> JPAC::fromMemory(std::span<const u8> buf, std::string_view path) {
  JPAC bin;
  TRY(bin.loadFromMemory(buf, path));
  return bin;
}

} // namespace librii::jpa
