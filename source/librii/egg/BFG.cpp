#include <rsl/Reflection.hpp>

#include "BFG.hpp"

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

} // namespace librii::egg
