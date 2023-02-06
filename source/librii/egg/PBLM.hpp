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
  u32 thresholdColor;
  ANNOTATE_STR2("@name", "compositeColor");
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

  bool operator==(const BLM&) const = default;
  bool operator!=(const BLM&) const = default;
};

inline Result<BLM> From_PBLM(const PBLM& b) {
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
  };
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
  };
}

} // namespace librii::egg
