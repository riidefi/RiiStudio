#include "Common.hpp"

namespace libcube::UI {

struct ColorSurface final {
  static inline const char* name = "Colors";
  static inline const char* icon = (const char*)ICON_FA_PAINT_BRUSH;
};

void drawProperty(kpi::PropertyDelegate<IGCMaterial>& delegate, ColorSurface) {
  librii::gx::ColorF32 clr;
  auto& matData = delegate.getActive().getMaterialData();

  if (ImGui::CollapsingHeader("TEV Color Registers",
                              ImGuiTreeNodeFlags_DefaultOpen)) {

    // TODO: Is CPREV default state accessible?
    for (std::size_t i = 0; i < 4; ++i) {
      clr = matData.tevColors[i];
      ImGui::ColorEdit4(
          (std::string("Color Register ") + std::to_string(i)).c_str(), clr);
      clr.clamp(-1024.0f / 255.0f, 1023.0f / 255.0f);
      AUTO_PROP(tevColors[i], static_cast<librii::gx::ColorS10>(clr));
    }
  }

  if (ImGui::CollapsingHeader("TEV Constant Colors",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    for (std::size_t i = 0; i < 4; ++i) {
      clr = matData.tevKonstColors[i];
      ImGui::ColorEdit4(
          (std::string("Constant Register ") + std::to_string(i)).c_str(), clr);
      clr.clamp(0.0f, 1.0f);
      AUTO_PROP(tevKonstColors[i], static_cast<librii::gx::Color>(clr));
    }
  }
}


kpi::RegisterPropertyView<IGCMaterial, ColorSurface>
    ColorSurfaceInstaller;

} // namespace libcube::UI
