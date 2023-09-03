
#include "JPAUtils.hpp"


namespace librii::jpa {

f32 JPAConvertFixToFloat(u16 number) { return (float)number / (float)0x8000; }

u16 JPAConvertFloatToFixed(f32 number) {
  if (number >= 1.0f) {
    return 0x7FFF;
  }

  return static_cast<short>(number * 0x8000);
}

f32* JPAConvertFixVectorToFloatVector(const u16 vector[3]) {
  f32* converted = new f32[3];
  converted[0] = JPAConvertFixToFloat(vector[0]);
  converted[1] = JPAConvertFixToFloat(vector[1]);
  converted[2] = JPAConvertFixToFloat(vector[2]);
  return converted;
}

u16* JPAConvertFloatVectorToFixVector(const f32 vector[3]) {
  u16* converted = new u16[3];
  converted[0] = JPAConvertFloatToFixed(vector[0]);
  converted[1] = JPAConvertFloatToFixed(vector[1]);
  converted[2] = JPAConvertFloatToFixed(vector[2]);
  return converted;
}

} // namespace librii::jpa
