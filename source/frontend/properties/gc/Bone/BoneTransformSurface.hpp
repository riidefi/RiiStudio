#pragma once

#include <LibBadUIFramework/PropertyView.hpp>
#include <imcxx/Widgets.hpp>
#include <plugins/gc/Export/Bone.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

namespace libcube::UI {

struct BoneTransformSurface {
  static inline const char* name() { return "Transformation"_j; }
  static inline const char* icon = (const char*)ICON_FA_ARROWS_ALT;
};

void drawProperty(kpi::PropertyDelegate<IBoneDelegate>& delegate,
                  BoneTransformSurface);

} // namespace libcube::UI
