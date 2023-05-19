#include <rsl/Reflection.hpp>

#include "BFG.hpp"

#include <core/util/oishii.hpp>

namespace librii::egg {

Result<BFG> BFG_Read(rsl::SafeReader& reader) {
  BFG result;
  for (size_t i = 0; i < 4; ++i) {
    BFG::Entry e{};
    TRY(rsl::ReadFields(e, reader));
    result.entries.push_back(e);
  }

  return result;
}

void BFG_Write(oishii::Writer& writer, const BFG& bfg) {
  for (auto& e : bfg.entries) {
    rsl::WriteFields(writer, e);
  }
}

Result<librii::egg::BFG> ReadBFG(std::span<const u8> buf,
                                 std::string_view path) {
  oishii::BinaryReader reader(buf, path, std::endian::big);
  rsl::SafeReader safe(reader);
  auto bblm = librii::egg::BFG_Read(safe);
  if (!bblm) {
    return std::unexpected("Failed to read BBLM: " + bblm.error());
  }
  return *bblm;
}
void WriteBFG(const librii::egg::BFG& b, std::string_view path) {
  rsl::trace("Attempting to save to {}", path);
  oishii::Writer writer(0);
  librii::egg::BFG_Write(writer, b);
  writer.saveToDisk(path);
}

} // namespace librii::egg
