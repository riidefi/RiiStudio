#include "bindings.h"

#include "WiiTrig.hpp"

f32 impl_wii_sin(f32 x) { return rlibrii::g3d::WiiSin(x); }
f32 impl_wii_cos(f32 x) { return rlibrii::g3d::WiiCos(x); }
