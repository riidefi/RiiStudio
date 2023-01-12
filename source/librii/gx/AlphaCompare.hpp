#pragma once

namespace librii::gx {
	

enum class AlphaOp { _and, _or, _xor, _xnor };

struct AlphaComparison {
  Comparison compLeft = Comparison::ALWAYS;
  u8 refLeft = 0;

  AlphaOp op = AlphaOp::_and;

  Comparison compRight = Comparison::ALWAYS;
  u8 refRight = 0;

  bool operator==(const AlphaComparison& rhs) const = default;
};

} // namepace librii::gx
