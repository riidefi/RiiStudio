#pragma once

#include <LibBadUIFramework/PropertyView.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <plugins/gc/Export/Material.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

#include <plugins/j3d/Joint.hpp>
#include <plugins/j3d/Material.hpp>
#include <plugins/j3d/Scene.hpp>
#include <plugins/j3d/Shape.hpp>

#include <imcxx/Widgets.hpp>

namespace riistudio::j3d::ui {

struct J3DDataSurface final {
  static inline const char* name() { return "J3D Data"_j; }
  static inline const char* icon = (const char*)ICON_FA_BOXES;
};

struct BoneJ3DSurface final {
  static inline const char* name() { return "J3D Data"_j; }
  static inline const char* icon = (const char*)ICON_FA_BOXES;
};

struct ShapeJ3DSurface final {
  static inline const char* name() { return "J3D Shape"_j; }
  static inline const char* icon = (const char*)ICON_FA_BOXES;
};

struct ModelJ3DSurface {
  static inline const char* name() { return "J3D Model"_j; }
  static inline const char* icon = (const char*)ICON_FA_ADDRESS_BOOK;
};

void drawProperty(kpi::PropertyDelegate<Material>& delegate, J3DDataSurface);
void drawProperty(kpi::PropertyDelegate<Joint>& delegate, BoneJ3DSurface);
void drawProperty(kpi::PropertyDelegate<Shape>& dl, ShapeJ3DSurface);
void drawProperty(kpi::PropertyDelegate<j3d::Model>& dl, ModelJ3DSurface);

} // namespace riistudio::j3d::ui
