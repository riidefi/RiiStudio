#pragma once

#include "../Enum/Comparison.hpp"

namespace libcube {
namespace gx {

struct ZMode {
  bool compare = true;
  Comparison function = Comparison::LEQUAL;
  bool update = true;

  const bool operator==(const ZMode& rhs) const noexcept {
    return compare == rhs.compare && function == rhs.function &&
           update == rhs.update;
  }
};

} // namespace gx
} // namespace libcube
