
#include "JPAUtils.hpp"


namespace librii::jpa {

f32 JPAConvertFixToFloat(u16 number) { return (float)number / (float)0x8000; }

u16 JPAConvertFloatToFixed(f32 number) {
  if (number >= 1.0f) {
    return 0x7FFF;
  }

  return static_cast<short>(number * 0x8000);
}

} // namespace librii::jpa
