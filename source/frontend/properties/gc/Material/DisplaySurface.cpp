#include "DisplaySurface.hpp"

namespace libcube::UI {

librii::gx::CullMode DrawCullMode(librii::gx::CullMode cull_mode) {
  librii::hx::CullMode widget(cull_mode);
  ImGui::TextUnformatted("Show sides of faces:"_j);
  ImGui::Checkbox("Front"_j, &widget.front);
  ImGui::Checkbox("Back"_j, &widget.back);
  return widget.get();
}

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate,
                  DisplaySurface) {
  mat_prop_ex(delegate, &GCMaterialData::cullMode, DrawCullMode);
}

} // namespace libcube::UI
