#pragma once

#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/PropertyView.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/g3d/collection.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>
#include <librii/g3d/io/AnimClrIO.hpp>

namespace riistudio::g3d {

struct G3dUniformAnimDataSurface {
  static inline const char* name() { return "Data"_j; }
  static inline const char* icon = (const char*)ICON_FA_COG;
};

void drawProperty(kpi::PropertyDelegate<riistudio::g3d::CLR0>& dl,
                 G3dUniformAnimDataSurface);

} // namespace riistudio::g3d
