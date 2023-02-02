#pragma once

#include <frontend/properties/gc/Material/Common.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/g3d/collection.hpp>
#include <plugins/g3d/material.hpp>

namespace libcube::UI {

struct LightingSurface final {
  static inline const char* name() { return "Lighting"_j; }
  static inline const char* icon = (const char*)ICON_FA_SUN;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  LightingSurface);

} // namespace libcube::UI
