#pragma once

#include "Common.hpp"
#include <imcxx/Widgets.hpp>
#include <librii/hx/PixMode.hpp>
#include <plugins/g3d/collection.hpp>

namespace libcube::UI {

struct PixelSurface final {
  static inline const char* name() { return "Pixel"_j; }
  static inline const char* icon = (const char*)ICON_FA_GHOST;

  int tag_stateful;

  std::string force_custom_at; // If empty, disabled: Name of material
  std::string force_custom_whole;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  PixelSurface& surface);

} // namespace libcube::UI
