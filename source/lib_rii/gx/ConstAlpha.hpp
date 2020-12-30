#pragma once

namespace librii::gx {
	

struct ConstantAlpha {
  bool enable = false;
  u8 value = 0;

  // void operator=(const GPU::CMODE1& reg);
};
	
} // namepace librii::gx