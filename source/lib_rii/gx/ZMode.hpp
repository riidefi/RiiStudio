#pragma once

namespace librii::gx {
	

struct ZMode {
  bool compare = true;
  Comparison function = Comparison::LEQUAL;
  bool update = true;

  const bool operator==(const ZMode& rhs) const noexcept {
    return compare == rhs.compare && function == rhs.function &&
           update == rhs.update;
  }
};
	
} // namepace librii::gx