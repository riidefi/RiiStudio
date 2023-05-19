#include <rsl/Reflection.hpp>

#include "BDOF.hpp"

#include <core/util/oishii.hpp>

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
  // EXPECT(b.magic == 'PDOF');
  // EXPECT(b.fileSize == 0x50);
  // EXPECT(b.revision == 0);
  // EXPECT(b._09 == 0);
  // EXPECT(b._A == 0);
  // EXPECT(b._C == 0);
  // EXPECT(b._40 == 0);
  // EXPECT(b._44 == 0);
  // EXPECT(b._48 == 0);
  // EXPECT(b._4C == 0);
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

Result<librii::egg::DOF> ReadDof(std::span<const u8> buf,
                                 std::string_view path) {
  oishii::BinaryReader reader(buf, path, std::endian::big);
  rsl::SafeReader safe(reader);
  auto bdof = librii::egg::bin::BDOF_Read(safe);
  if (!bdof) {
    return std::unexpected("Failed to read BDOF: " + bdof.error());
  }
  return librii::egg::From_BDOF(*bdof);
}
oishii::Writer WriteDofMemory(const librii::egg::DOF& b) {
  oishii::Writer writer(std::endian::big);
  auto b2 = To_BDOF(b);
  bin::BDOF_Write(writer, b2);
  return writer;
}
void WriteDof(const librii::egg::DOF& b, std::string_view path) {
  rsl::trace("Attempting to save to {}", path);
  auto writer = WriteDofMemory(b);
  writer.saveToDisk(path);
}

} // namespace librii::egg
