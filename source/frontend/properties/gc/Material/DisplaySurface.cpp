#include "Common.hpp"
#include <librii/hx/CullMode.hpp>

namespace libcube::UI {

librii::gx::CullMode DrawCullMode(librii::gx::CullMode cull_mode) {
  librii::hx::CullMode widget(cull_mode);
  ImGui::TextUnformatted("Show sides of faces:"_j);
  ImGui::Checkbox("Front"_j, &widget.front);
  ImGui::Checkbox("Back"_j, &widget.back);
  return widget.get();
}

struct DisplaySurface final {
  static inline const char* name() { return "Surface Visibility"_j; }
  static inline const char* icon = (const char*)ICON_FA_GHOST;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  DisplaySurface) {
  mat_prop_ex(delegate, &GCMaterialData::cullMode, DrawCullMode);
}

kpi::RegisterPropertyView<IGCMaterial, DisplaySurface> DisplaySurfaceInstaller;

} // namespace libcube::UI
