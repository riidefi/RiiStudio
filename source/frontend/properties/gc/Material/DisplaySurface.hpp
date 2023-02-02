#pragma once

#include <frontend/properties/gc/Material/Common.hpp>
#include <librii/hx/CullMode.hpp>

namespace libcube::UI {

struct DisplaySurface final {
  static inline const char* name() { return "Surface Visibility"_j; }
  static inline const char* icon = (const char*)ICON_FA_GHOST;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate, DisplaySurface);

} // namespace libcube::UI
