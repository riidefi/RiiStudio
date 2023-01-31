#include <LibBadUIFramework/PropertyView.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <plugins/gc/Export/Material.hpp>
#include <vendor/fa5/IconsFontAwesome5.h>

#include <plugins/g3d/collection.hpp>

#include <imcxx/Widgets.hpp>

namespace riistudio::g3d::ui {

struct G3DDataSurface final {
  static inline const char* name() { return "Fog"; }
  static inline const char* icon = (const char*)ICON_FA_SHIP;
};

void drawProperty(kpi::PropertyDelegate<Material>& delegate, G3DDataSurface) {
  auto* gm = &delegate.getActive();
  bool used = gm->fogIndex >= 0;
  bool input = ImGui::Checkbox("Use scene fog?", &used);
  riistudio::util::ConditionalActive ca(used);
  int fog = std::max<int>(0, gm->fogIndex);
  if (!used) {
    fog = -1;
  }
  ImGui::SameLine();
  input |= ImGui::InputInt("index", &fog);
  fog = std::min<int>(127, fog);
  if (!used) {
    fog = -1;
  }
  if (input && gm->fogIndex != fog) {
    for (auto& a : delegate.mAffected) {
      if (auto* g = dynamic_cast<riistudio::g3d::Material*>(a)) {
        g->fogIndex = fog;
        g->onUpdate();
      }
    }
    delegate.commit("Fog update");
  }
}

struct G3DTexDataSurface final {
  static inline const char* name() { return "BRRES Data"; }
  static inline const char* icon = (const char*)ICON_FA_BOXES;
};

void drawProperty(kpi::PropertyDelegate<Texture>& delegate, G3DTexDataSurface) {
  bool custom_lod = delegate.getActive().custom_lod;
  ImGui::Checkbox("Custom Quality Range", &custom_lod);
  KPI_PROPERTY_EX(delegate, custom_lod, custom_lod);

  riistudio::util::ConditionalActive g(custom_lod);

  float min = delegate.getActive().minLod;
  ImGui::InputFloat("Maximum mipmap quality", &min);
  KPI_PROPERTY_EX(delegate, minLod, min);

  float max = delegate.getActive().maxLod;
  ImGui::InputFloat("Maximum mipmap quality", &max);
  KPI_PROPERTY_EX(delegate, maxLod, max);
}
struct G3DMdlDataSurface final {
  static inline const char* name() { return "BRRES Data"; }
  static inline const char* icon = (const char*)ICON_FA_BOXES;
};

void drawProperty(kpi::PropertyDelegate<Model>& delegate, G3DMdlDataSurface) {
  auto& active = delegate.getActive();
  auto [nVert, nTri] = librii::gx::computeVertTriCounts(active.getMeshes());
  ImGui::Text("Number of facepoints: %u\n", nVert);
  ImGui::Text("Number of faces: %u\n", nTri);
}

kpi::DecentralizedInstaller Installer([](kpi::ApplicationPlugins&) {
  auto& inst = kpi::PropertyViewManager::getInstance();
  inst.addPropertyView<Model, G3DMdlDataSurface>();
  inst.addPropertyView<Material, G3DDataSurface>();
  inst.addPropertyView<Texture, G3DTexDataSurface, true>();
});

} // namespace riistudio::g3d::ui
