#include <rsl/Reflection.hpp>

#include "BDOF.hpp"

namespace librii::egg {

namespace bin {
Result<BDOF> BDOF_Read(rsl::SafeReader& reader) {
  BDOF bdof{};
  TRY(rsl::ReadFields(bdof, reader));
  return bdof;
}

void BDOF_Write(oishii::Writer& writer, const BDOF& bdof) {
  rsl::WriteFields(writer, bdof);
}
} // namespace bin

Result<DOF> From_BDOF(const bin::BDOF& b) {
  EXPECT(b.magic == 'PDOF');
  EXPECT(b.fileSize == 0x50);
  EXPECT(b.revision == 0);
  EXPECT(b._09 == 0);
  EXPECT(b._A == 0);
  EXPECT(b._C == 0);
  EXPECT(b._40 == 0);
  EXPECT(b._44 == 0);
  EXPECT(b._48 == 0);
  EXPECT(b._4C == 0);
  return DOF{
      .flags = b.flags,
      .blurAlpha0 = b.blurAlpha0,
      .blurAlpha1 = b.blurAlpha1,
      .drawMode = TRY(rsl::enum_cast<DOF::DrawMode>(b.drawMode)),
      .blurDrawAmount =
          TRY(rsl::enum_cast<DOF::BlurDrawAmount>(b.blurDrawAmount)),
      .depthCurveType = b.depthCurveType,
      ._17 = b._17,
      .focusCenter = b.focusCenter,
      .focusRange = b.focusRange,
      ._20 = b._20,
      .blurRadius = b.blurRadius,
      .indTexTransSScroll = b.indTexTransSScroll,
      .indTexTransTScroll = b.indTexTransTScroll,
      .indTexIndScaleS = b.indTexIndScaleS,
      .indTexIndScaleT = b.indTexIndScaleT,
      .indTexScaleS = b.indTexScaleS,
      .indTexScaleT = b.indTexScaleT,
  };
}
bin::BDOF To_BDOF(const DOF& b) {
  return bin::BDOF{
      .flags = b.flags,
      .blurAlpha0 = b.blurAlpha0,
      .blurAlpha1 = b.blurAlpha1,
      .drawMode = static_cast<u8>(b.drawMode),
      .blurDrawAmount = static_cast<u8>(b.blurDrawAmount),
      .depthCurveType = b.depthCurveType,
      ._17 = b._17,
      .focusCenter = b.focusCenter,
      .focusRange = b.focusRange,
      ._20 = b._20,
      .blurRadius = b.blurRadius,
      .indTexTransSScroll = b.indTexTransSScroll,
      .indTexTransTScroll = b.indTexTransTScroll,
      .indTexIndScaleS = b.indTexIndScaleS,
      .indTexIndScaleT = b.indTexIndScaleT,
      .indTexScaleS = b.indTexScaleS,
      .indTexScaleT = b.indTexScaleT,
  };
}

} // namespace librii::egg
