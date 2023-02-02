#pragma once

#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/PropertyView.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/g3d/collection.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::g3d {

struct G3dSrtOptionsSurface {
  static inline const char* name() { return "Options"_j; }
  static inline const char* icon = (const char*)ICON_FA_COG;
};

void drawProperty(kpi::PropertyDelegate<SRT0>& dl, G3dSrtOptionsSurface);

} // namespace riistudio::g3d
