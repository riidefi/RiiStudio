#pragma once

namespace librii::jpa {


f32 JPAConvertFixToFloat(u16 number);

u16 JPAConvertFloatToFixed(f32 number);


f32* JPAConvertFixVectorToFloatVector(const u16 vector[3]);

u16* JPAConvertFloatVectorToFixVector(const f32 vector[3]);

} // namespace librii::jpa
