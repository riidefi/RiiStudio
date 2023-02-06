#include <rsl/Reflection.hpp>

#include "PBLM.hpp"

namespace librii::egg {

Result<PBLM> PBLM_Read(rsl::SafeReader& reader) {
  PBLM pblm{};
  TRY(rsl::ReadFields(pblm, reader));
  return pblm;
}

void PBLM_Write(oishii::Writer& writer, const PBLM& pblm) {
  rsl::WriteFields(writer, pblm);
}

} // namespace librii::egg
