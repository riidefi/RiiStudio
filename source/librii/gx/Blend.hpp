#pragma once

#include <core/common.h>

namespace librii::gx {

enum class BlendModeType { none, blend, logic, subtract };

enum class BlendModeFactor {
  zero,
  one,
  src_c,
  inv_src_c,
  src_a,
  inv_src_a,
  dst_a,
  inv_dst_a
};

enum class LogicOp {
  _clear,
  _and,
  _rev_and,
  _copy,
  _inv_and,
  _no_op,
  _xor,
  _or,
  _nor,
  _equiv,
  _inv,
  _revor,
  _inv_copy,
  _inv_or,
  _nand,
  _set
};

// CMODE0
struct BlendMode {
  BlendModeType type = BlendModeType::none;
  BlendModeFactor source = BlendModeFactor::src_a;
  BlendModeFactor dest = BlendModeFactor::inv_src_a;
  LogicOp logic = LogicOp::_copy;

  bool operator==(const BlendMode& rhs) const = default;
};
// CMODE1
struct DstAlpha {
  bool enabled = false;
  u8 alpha = 0;

  bool operator==(const DstAlpha&) const = default;
};

} // namespace librii::gx
