#pragma once

#include <core/common.h>
#include <oishii/writer/binary_writer.hxx>
#include <rsl/Annotations.hpp>
#include <rsl/SafeReader.hpp>

namespace librii::egg {

struct PBLM {
  u32 magic = 'PBLM';
  u32 fileSize = 0xA4;
  u8 version = 0x1;
  u8 _9 = 0;
  u16 _a = 0;
  u32 _c = 0;

  f32 thresholdAmount;
  u32 thresholdColor;
  u32 compositeColor;
  u16 blurFlags;
  u16 _1e = 0;
  f32 blur0Radius;
  f32 blur0Intensity;
  u32 _28 = 0;
  u32 _2c = 0;
  u32 _30 = 0;
  u32 _34 = 0;
  u32 _38 = 0;
  u32 _3c = 0;
  f32 blur1Radius;
  f32 blur1Intensity;
  u32 _48 = 0;
  u32 _4c = 0;
  u32 _50 = 0;
  u32 _54 = 0;
  u32 _58 = 0;
  u32 _5c = 0;
  f32 _60;
  f32 _64;
  u32 _68 = 0;
  u32 _6c = 0;
  u32 _70 = 0;
  u32 _74 = 0;
  u32 _78 = 0;
  u32 _7c = 0;
  u8 compositeBlendMode;
  u8 blur1NumPasses;
  u16 _82 = 0;
  u32 _84 = 0;
  u32 _88 = 0;
  u32 _8c = 0;
  u32 _90 = 0;
  u32 _94 = 0;
  u32 _98 = 0;
  f32 bokehColorScale0;
  f32 bokehColorScale1;
};
static_assert(sizeof(PBLM) == 0xA4);

Result<PBLM> PBLM_Read(rsl::SafeReader& reader);
void PBLM_Write(oishii::Writer& writer, const PBLM& pblm);

struct BLM {
  ANNOTATE_STR2("@name", "thresholdAmount");
  f32 thresholdAmount;
  ANNOTATE_STR2("@name", "thresholdColor");
  ANNOTATE_STR2("@type", "color32");
  u32 thresholdColor;
  ANNOTATE_STR2("@name", "compositeColor");
  ANNOTATE_STR2("@type", "color32");
  u32 compositeColor;
  ANNOTATE_STR2("@name", "blurFlags");
  u16 blurFlags;
  ANNOTATE_STR2("@name", "blur0Radius");
  f32 blur0Radius;
  ANNOTATE_STR2("@name", "blur0Intensity");
  f32 blur0Intensity;
  ANNOTATE_STR2("@name", "blur1Radius");
  f32 blur1Radius;
  ANNOTATE_STR2("@name", "blur1Intensity");
  f32 blur1Intensity;

  
  ANNOTATE_STR2("@name", "_60");
  f32 _60;
  ANNOTATE_STR2("@name", "_64");
  f32 _64;

  ANNOTATE_STR2("@name", "compositeBlendMode");
  u8 compositeBlendMode;
  ANNOTATE_STR2("@name", "blur1NumPasses");
  u8 blur1NumPasses;
  ANNOTATE_STR2("@name", "bokehColorScale0");
  f32 bokehColorScale0;
  ANNOTATE_STR2("@name", "bokehColorScale1");
  f32 bokehColorScale1;

  struct Unks {
    u16 _1e = 0;
    u32 _28 = 0;
    u32 _2c = 0;
    u32 _30 = 0;
    u32 _34 = 0;
    u32 _38 = 0;
    u32 _3c = 0;
    u32 _48 = 0;
    u32 _4c = 0;
    u32 _50 = 0;
    u32 _54 = 0;
    u32 _58 = 0;
    u32 _5c = 0;
    u32 _68 = 0;
    u32 _6c = 0;
    u32 _70 = 0;
    u32 _74 = 0;
    u32 _78 = 0;
    u32 _7c = 0;
    u16 _82 = 0;
    u32 _84 = 0;
    u32 _88 = 0;
    u32 _8c = 0;
    u32 _90 = 0;
    u32 _94 = 0;
    u32 _98 = 0;
    bool operator==(const Unks&) const = default;
  } unks;

  bool operator==(const BLM&) const = default;
  bool operator!=(const BLM&) const = default;
};

inline Result<BLM> From_PBLM(const PBLM& b) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreorder-init-list"
  return BLM{
      .thresholdAmount = b.thresholdAmount,
      .thresholdColor = b.thresholdColor,
      .compositeColor = b.compositeColor,
      .blurFlags = b.blurFlags,
      .blur0Radius = b.blur0Radius,
      .blur0Intensity = b.blur0Intensity,
      .blur1Radius = b.blur1Radius,
      .blur1Intensity = b.blur1Intensity,
      ._60 = b._60,
      ._64 = b._64,
      .compositeBlendMode = b.compositeBlendMode,
      .blur1NumPasses = b.blur1NumPasses,
      .bokehColorScale0 = b.bokehColorScale0,
      .bokehColorScale1 = b.bokehColorScale1,
      // Unknown fields
      .unks._1e = b._1e,
      .unks._28 = b._28,
      .unks._2c = b._2c,
      .unks._30 = b._30,
      .unks._34 = b._34,
      .unks._38 = b._38,
      .unks._3c = b._3c,
      .unks._48 = b._48,
      .unks._4c = b._4c,
      .unks._50 = b._50,
      .unks._54 = b._54,
      .unks._58 = b._58,
      .unks._5c = b._5c,
      .unks._68 = b._68,
      .unks._6c = b._6c,
      .unks._70 = b._70,
      .unks._74 = b._74,
      .unks._78 = b._78,
      .unks._7c = b._7c,
      .unks._82 = b._82,
      .unks._84 = b._84,
      .unks._88 = b._88,
      .unks._8c = b._8c,
      .unks._90 = b._90,
      .unks._94 = b._94,
      .unks._98 = b._98,
  };
#pragma clang diagnostic pop
}
inline PBLM To_PBLM(const BLM& b) {
  return PBLM{
      .thresholdAmount = b.thresholdAmount,
      .thresholdColor = b.thresholdColor,
      .compositeColor = b.compositeColor,
      .blurFlags = b.blurFlags,
      .blur0Radius = b.blur0Radius,
      .blur0Intensity = b.blur0Intensity,
      .blur1Radius = b.blur1Radius,
      .blur1Intensity = b.blur1Intensity,
      ._60 = b._60,
      ._64 = b._64,
      .compositeBlendMode = b.compositeBlendMode,
      .blur1NumPasses = b.blur1NumPasses,
      .bokehColorScale0 = b.bokehColorScale0,
      .bokehColorScale1 = b.bokehColorScale1,
      // Unknown fields
      ._1e = b.unks._1e,
      ._28 = b.unks._28,
      ._2c = b.unks._2c,
      ._30 = b.unks._30,
      ._34 = b.unks._34,
      ._38 = b.unks._38,
      ._3c = b.unks._3c,
      ._48 = b.unks._48,
      ._4c = b.unks._4c,
      ._50 = b.unks._50,
      ._54 = b.unks._54,
      ._58 = b.unks._58,
      ._5c = b.unks._5c,
      ._68 = b.unks._68,
      ._6c = b.unks._6c,
      ._70 = b.unks._70,
      ._74 = b.unks._74,
      ._78 = b.unks._78,
      ._7c = b.unks._7c,
      ._82 = b.unks._82,
      ._84 = b.unks._84,
      ._88 = b.unks._88,
      ._8c = b.unks._8c,
      ._90 = b.unks._90,
      ._94 = b.unks._94,
      ._98 = b.unks._98,
  };
}

} // namespace librii::egg
