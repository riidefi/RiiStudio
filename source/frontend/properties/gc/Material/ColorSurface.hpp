#pragma once

#include <frontend/properties/gc/Material/Common.hpp>

namespace libcube::UI {

struct ColorSurface final {
  static inline const char* name() { return "Colors"_j; }
  static inline const char* icon = (const char*)ICON_FA_PAINT_BRUSH;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate, ColorSurface);

} // namespace libcube::UI
