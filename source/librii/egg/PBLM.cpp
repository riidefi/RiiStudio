#include <rsl/Reflection.hpp>

#include "PBLM.hpp"

#include <core/util/oishii.hpp>

namespace librii::egg {

Result<PBLM> PBLM_Read(rsl::SafeReader& reader) {
  PBLM pblm{};
  TRY(rsl::ReadFields(pblm, reader));
  return pblm;
}

void PBLM_Write(oishii::Writer& writer, const PBLM& pblm) {
  rsl::WriteFields(writer, pblm);
}

Result<librii::egg::BLM> ReadBLM(std::span<const u8> buf,
                                 std::string_view path) {
  oishii::BinaryReader reader(buf, path, std::endian::big);
  rsl::SafeReader safe(reader);
  auto bblm = librii::egg::PBLM_Read(safe);
  if (!bblm) {
    return std::unexpected("Failed to read BBLM: " + bblm.error());
  }
  auto blm = librii::egg::From_PBLM(*bblm);
  if (!blm) {
    return std::unexpected("Failed to parse BBLM: " + blm.error());
  }
  return *blm;
}
void WriteBLM(const librii::egg::BLM& b, std::string_view path) {
  rsl::trace("Attempting to save to {}", path);
  oishii::Writer writer(0x50);
  auto bblm = librii::egg::To_PBLM(b);
  librii::egg::PBLM_Write(writer, bblm);
  writer.saveToDisk(path);
}

} // namespace librii::egg
