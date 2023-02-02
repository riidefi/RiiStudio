#pragma once

#include "Common.hpp"
#include <imcxx/Widgets.hpp>
#include <plugins/gc/Export/Scene.hpp>

namespace libcube::UI {

struct IndirectSurface final {
  static inline const char* name() { return "Displacement"_j; }
  static inline const char* icon = (const char*)ICON_FA_WATER;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  IndirectSurface);

} // namespace libcube::UI
