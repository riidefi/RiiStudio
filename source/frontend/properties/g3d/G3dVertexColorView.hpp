#pragma once

#include <LibBadUIFramework/Node2.hpp>
#include <LibBadUIFramework/PropertyView.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/g3d/collection.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace riistudio::g3d {

struct G3dVertexColorDataSurface {
  static inline const char* name() { return "Data"_j; }
  static inline const char* icon = (const char*)ICON_FA_PAINT_BRUSH;
};
struct G3dVertexColorQuantSurface {
  static inline const char* name() { return "Format"_j; }
  static inline const char* icon = (const char*)ICON_FA_COG;
};

void drawProperty(kpi::PropertyDelegate<ColorBuffer>& dl,
                  G3dVertexColorQuantSurface);
void drawProperty(kpi::PropertyDelegate<ColorBuffer>& dl,
                  G3dVertexColorDataSurface);

} // namespace riistudio::g3d
