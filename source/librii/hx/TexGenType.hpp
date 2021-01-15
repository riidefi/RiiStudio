#pragma once

#include <librii/gx.h>

namespace librii::hx {

enum class BaseTexGenFunction : int {
  TextureMatrix, //!< Standard configuration: Use mtxtype
  Bump,
  SRTG //!< Map R(ed) and G(reen) components of a color channel to U/V
       //!< coordinates
};

struct TexGenType {
  int basefunc = 0; // BaseTexGenFunction
  int mtxtype = 0;  //!< When basefunc == TextureMatrix
  int lightid = 0;  //!< When basefunc == Bump

  bool operator==(const TexGenType&) const = default;
};

inline TexGenType elevateTexGenType(gx::TexGenType func) {
  TexGenType result;
  switch (func) {
  case gx::TexGenType::Matrix2x4:
    result.basefunc = 0;
    result.mtxtype = 0;
    break;
  case gx::TexGenType::Matrix3x4:
    result.basefunc = 0;
    result.mtxtype = 1;
    break;
  case gx::TexGenType::Bump0:
  case gx::TexGenType::Bump1:
  case gx::TexGenType::Bump2:
  case gx::TexGenType::Bump3:
  case gx::TexGenType::Bump4:
  case gx::TexGenType::Bump5:
  case gx::TexGenType::Bump6:
  case gx::TexGenType::Bump7:
    result.basefunc = 1;
    result.lightid =
        static_cast<int>(func) - static_cast<int>(gx::TexGenType::Bump0);
    break;
  case gx::TexGenType::SRTG:
    result.basefunc = 2;
    break;
  }

  return result;
}

inline gx::TexGenType lowerTexGenType(hx::TexGenType tg) {
  switch (tg.basefunc) {
  case 0:
  default:
    return tg.mtxtype ? gx::TexGenType::Matrix3x4 : gx::TexGenType::Matrix2x4;
  case 1:
    return static_cast<gx::TexGenType>(static_cast<int>(gx::TexGenType::Bump0) +
                                       tg.lightid);
  case 2:
    return gx::TexGenType::SRTG;
  }
}

} // namespace librii::hx
