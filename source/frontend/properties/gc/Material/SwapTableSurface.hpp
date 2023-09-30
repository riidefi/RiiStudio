#pragma once

#include <frontend/properties/gc/Material/Common.hpp>
#include <imcxx/Widgets.hpp>

namespace libcube::UI {

struct SwapTableSurface final {
  static inline const char* name() { return "Swizzling"_j; }
  static inline const char* icon = (const char*)ICON_FA_SWATCHBOOK;
  static inline ImVec4 color = Clr(0x00AAE7);
};
void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  SwapTableSurface);

} // namespace libcube::UI
