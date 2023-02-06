#pragma once

#include <core/common.h>
#include <oishii/writer/binary_writer.hxx>
#include <rsl/Annotations.hpp>
#include <rsl/SafeReader.hpp>

namespace librii::egg {

namespace bin {
struct BDOF {
  u32 magic = 'PDOF';
  u32 fileSize = 0x50;
  u8 revision = 0;
  u8 _09 = 0;
  u16 _A = 0;
  u32 _C = 0;
  u16 flags;
  u8 blurAlpha0;
  u8 blurAlpha1;
  u8 drawMode;
  u8 blurDrawAmount;
  u8 depthCurveType;
  u8 _17;
  f32 focusCenter;
  f32 focusRange;
  f32 _20;
  f32 blurRadius;
  f32 indTexTransSScroll;
  f32 indTexTransTScroll;
  f32 indTexIndScaleS;
  f32 indTexIndScaleT;
  f32 indTexScaleS;
  f32 indTexScaleT;
  u32 _40 = 0;
  u32 _44 = 0;
  u32 _48 = 0;
  u32 _4C = 0;
};
static_assert(sizeof(BDOF) == 0x50);

Result<BDOF> BDOF_Read(rsl::SafeReader& reader);
void BDOF_Write(oishii::Writer& writer, const BDOF& bdof);
} // namespace bin

struct DOF {
  enum Flags {
    FLAG_USE_IND_WARP = (1 << 1),
  };

  enum class DrawMode {
    DRAWMODE0,
    DRAWMODE1,
    DRAWMODE2,
  };

  enum class BlurDrawAmount {
    TAPS_4,
    TAPS_2,
  };

  ANNOTATE_STR2("@name", "Flags");
  u16 flags = 0;

  ANNOTATE_STR2("@name", "Blur Alpha 0");
  u8 blurAlpha0;
  ANNOTATE_STR2("@name", "Blur Alpha 1");
  u8 blurAlpha1;

  ANNOTATE_STR2("@name", "Draw Mode");
  DrawMode drawMode;

  ANNOTATE_STR2("@name", "Number of blur taps ");
  BlurDrawAmount blurDrawAmount;

  ANNOTATE_STR2("@name", "Depth curve type");
  u8 depthCurveType;

  ANNOTATE_STR2("@name", "Unk 0x17");
  u8 _17;

  ANNOTATE_STR2("@name", "Focus Center");
  f32 focusCenter;

  ANNOTATE_STR2("@name", "Focus Range");
  f32 focusRange;

  ANNOTATE_STR2("@name", "Unk 0x20");
  f32 _20;

  ANNOTATE_STR2("@name", "Blur Radius");
  f32 blurRadius;

  ANNOTATE_STR2("@name", "indTexTransSScroll");
  f32 indTexTransSScroll;
  ANNOTATE_STR2("@name", "indTexTransTScroll");
  f32 indTexTransTScroll;

  ANNOTATE_STR2("@name", "indTexIndScaleS");
  f32 indTexIndScaleS;
  ANNOTATE_STR2("@name", "indTexIndScaleT");
  f32 indTexIndScaleT;

  ANNOTATE_STR2("@name", "indTexScaleS");
  f32 indTexScaleS;
  ANNOTATE_STR2("@name", "indTexScaleT");
  f32 indTexScaleT;

  bool operator==(const DOF&) const = default;
  bool operator!=(const DOF&) const = default;
};

Result<DOF> From_BDOF(const bin::BDOF& b);
bin::BDOF To_BDOF(const DOF& b);

} // namespace librii::egg
