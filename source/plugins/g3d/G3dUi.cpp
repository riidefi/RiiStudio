#include <core/kpi/PropertyView.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <plugins/gc/Export/Material.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

#include <plugins/g3d/collection.hpp>

#include <core/util/gui.hpp>
#include <imcxx/Widgets.hpp>

namespace riistudio::g3d::ui {

struct G3DDataSurface final {
  static inline const char* name = "BRRES Data";
  static inline const char* icon = (const char*)ICON_FA_BOXES;
};

void drawProperty(kpi::PropertyDelegate<Material>& delegate, G3DDataSurface) {
  int lightset = delegate.getActive().lightSetIndex;
  ImGui::InputInt("Light channel (-1 == OFF)", &lightset);
  KPI_PROPERTY_EX(delegate, lightSetIndex, static_cast<s8>(lightset));

  int fog = delegate.getActive().fogIndex;
  ImGui::InputInt("Fog channel (-1 == OFF)", &fog);
  KPI_PROPERTY_EX(delegate, fogIndex, static_cast<s8>(fog));
}

kpi::DecentralizedInstaller Installer([](kpi::ApplicationPlugins&) {
  auto& inst = kpi::PropertyViewManager::getInstance();
  inst.addPropertyView<Material, G3DDataSurface>();
});

} // namespace riistudio::g3d::ui
