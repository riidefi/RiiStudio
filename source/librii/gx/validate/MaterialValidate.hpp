#pragma once

#include <librii/gx.h>

namespace librii::gx {

//! Expand shared texgens across samplers.
//! This does not handle the case where samplers are shared across texgens.
//!
void expandSharedTexGens(LowLevelGxMaterial& material);

} // namespace librii::gx
