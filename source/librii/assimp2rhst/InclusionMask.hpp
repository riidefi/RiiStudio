#pragma once

#include <core/common.h>

namespace librii::assimp2rhst {

// Print an exclusion mask
void DebugPrintExclusionMask(u32 exclusion_mask);

// Turn an inclusion mask into an exclusion mask
u32 FlipExclusionMask(u32 exclusion_mask);

// Default attributes to be included in a model import
u32 DefaultInclusionMask();


} // namespace librii::assimp2rhst
