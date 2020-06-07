#pragma once

#include "core/common.h"
#include <array>

namespace riistudio::lib3d {

enum class Coverage { Unsupported, Read, ReadWrite };

template <typename Feature> struct TPropertySupport {

  Coverage supports(Feature f) const noexcept {
    assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

    if (static_cast<u64>(f) >= static_cast<u64>(Feature::Max))
      return Coverage::Unsupported;

    return registration[static_cast<u64>(f)];
  }
  bool canRead(Feature f) const noexcept {
    return supports(f) == Coverage::Read || supports(f) == Coverage::ReadWrite;
  }
  bool canWrite(Feature f) const noexcept {
    return supports(f) == Coverage::ReadWrite;
  }
  void setSupport(Feature f, Coverage s) {
    assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

    if (static_cast<u64>(f) < static_cast<u64>(Feature::Max))
      registration[static_cast<u64>(f)] = s;
  }

  Coverage& operator[](Feature f) noexcept {
    assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

    return registration[static_cast<u64>(f)];
  }
  Coverage operator[](Feature f) const noexcept {
    assert(static_cast<u64>(f) < static_cast<u64>(Feature::Max));

    return registration[static_cast<u64>(f)];
  }

private:
  std::array<Coverage, static_cast<u64>(Feature::Max)> registration;
};

} // namespace riistudio::lib3d
