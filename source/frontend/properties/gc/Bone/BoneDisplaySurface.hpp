#pragma once

#include <LibBadUIFramework/PropertyView.hpp>
#include <imcxx/Widgets.hpp>
#include <librii/g3d/data/BoneData.hpp>
#include <plugins/g3d/material.hpp>
#include <plugins/gc/Export/Bone.hpp>
#include <plugins/gc/Export/IndexedPolygon.hpp>
#include <plugins/gc/Export/Material.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace libcube::UI {

struct BoneDisplaySurface {
  static inline const char* name() { return "Displays"_j; }
  static inline const char* icon = (const char*)ICON_FA_IMAGE;
};

void drawProperty(kpi::PropertyDelegate<libcube::IBoneDelegate>& dl,
                  BoneDisplaySurface);

} // namespace libcube::UI
