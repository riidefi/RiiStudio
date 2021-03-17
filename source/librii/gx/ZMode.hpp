#pragma once

namespace librii::gx {
	

struct ZMode {
  bool compare = true;
  Comparison function = Comparison::LEQUAL;
  bool update = true;

  bool operator==(const ZMode& rhs) const noexcept = default;
};
	
} // namepace librii::gx
